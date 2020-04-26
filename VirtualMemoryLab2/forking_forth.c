#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stdbool.h>
#include <signal.h>
#include <sys/mman.h>
#include <unistd.h>
#include <string.h>
#include "forth_embed.h"
#include "forking_forth.h"

// if the region requested is already mapped, things fail
// so we want address that won't get used as the program
// starts up
#define UNIVERSAL_PAGE_START 0xf9f8c000


// the number of memory pages will will allocate to an instance of forth
#define NUM_PAGES 12 // last two pages are for the return stack
#define MAX_FORTHS 10

#define PAGE_UNCREATED -1

struct forth_extra_data {
    bool valid;
    struct forth_data data;
	int page_table[NUM_PAGES];
};

struct forth_extra_data forth_extra_data[MAX_FORTHS];  

// GLOBALS
int used_pages_count;
int frames_fd; // file descriptor for frames array
char* frames; // large mmaped region that forths will share
int forth_idx; // tracks current forth instance
int frame_idx = 0; // tracks location in global frame array
int share_count[NUM_PAGES*MAX_FORTHS]; // tracks shared frames

int get_used_pages_count() {
    return used_pages_count;
}

// finds next available slot for a forth instance
int find_available_slot() {
    int forth_num;
    for(forth_num = 0; forth_num < MAX_FORTHS; forth_num++) {
        if(forth_extra_data[forth_num].valid == false) {
            break; // we've found a num to use
        }
    }
    if(forth_num == MAX_FORTHS) {
        printf("We've created too many forths!");
        exit(1);
    }
    return forth_num;
}

// switches universal region to the given forth instance
void switch_current_to(int forthnum) {
	forth_idx = forthnum; // set global index
	
	// munmap the universal array first
	int munmap_result = munmap((void*) UNIVERSAL_PAGE_START, getpagesize()*NUM_PAGES);
	if (munmap_result < 0) {
		perror("munmap failed");
		exit(6);
	} 
	
	// iterate through page_table and map to big chunk
	// this only restores the pages to universal memory that were previously mapped by this forth instance
	for (int i=0; i<NUM_PAGES; i++) {
		if (forth_extra_data[forth_idx].page_table[i] != -1) {
			int frame_num = forth_extra_data[forth_idx].page_table[i];
			void* desired_addr = (void*) UNIVERSAL_PAGE_START + (getpagesize()*i);
			void* result;

			// if this frame is shared, mark as read only
			if (share_count[frame_num] > 1) {
				result = mmap(desired_addr,
							getpagesize(),
							PROT_READ | PROT_EXEC,
							MAP_SHARED | MAP_FIXED,
							frames_fd,
							frame_num*getpagesize());
			}

			// if this frame is not shared, mark as write and read
			else {
				result = mmap(desired_addr,
							getpagesize(),
							PROT_READ | PROT_WRITE | PROT_EXEC,
							MAP_SHARED | MAP_FIXED,
							frames_fd,
							frame_num*getpagesize());
			}

			if (result == MAP_FAILED) {
				perror("map failed");
				exit(1);
			}
		}	
	}
	// STEP 0-1
	// this maps an entire forth section at once to the global frame array
	/*
	int offset = forthnum*getpagesize()*NUM_PAGES;
	curr_forth = mmap((void*) UNIVERSAL_PAGE_START,
					getpagesize()*NUM_PAGES,
					PROT_READ | PROT_WRITE | PROT_EXEC,
					MAP_SHARED | MAP_FIXED,
					frames_fd,
					offset);
	if(curr_forth == MAP_FAILED) {
		perror("map failed");
		exit(1);
	}
	*/
}

// function to fork the current process. Returns child process id
int fork_forth(int forknum) {
	int parent = forknum;
	int child_id = find_available_slot();
	forth_extra_data[child_id].valid = true;

	// copy forth data from parent
	void* result = memcpy(&forth_extra_data[child_id].data,
					&forth_extra_data[parent].data,
					sizeof(struct forth_data));
	if (result != &forth_extra_data[child_id].data) {
		perror("memcpy failed");
		exit(4);
	}


	// --allocate new frames for child--
	// EDIT STEP 5: Only copy on write
	for (int i=0; i<NUM_PAGES; i++) {
		if (forth_extra_data[parent].page_table[i] != -1) {
			int frame_num = forth_extra_data[parent].page_table[i];
			forth_extra_data[child_id].page_table[i] = frame_num;
			share_count[frame_num]++;
			/* EDIT STEP 5
			void* page_data = (void*) frames + (frame_num*getpagesize());
			void* desired_addr = (void*) frames + (frame_idx*getpagesize());
			// copy parent page data into child's frame
			void* result = memcpy(desired_addr,page_data,getpagesize());
			if (result != desired_addr) {
				perror("memcpy failed");
				exit(4);
			}
			// map child frame in universal region
			desired_addr = (void*) UNIVERSAL_PAGE_START + (getpagesize()*i);
			result = mmap(desired_addr,
							getpagesize(),
							PROT_READ | PROT_WRITE | PROT_EXEC,
							MAP_SHARED | MAP_FIXED,
							frames_fd,
							frame_idx*getpagesize());
			if (result == MAP_FAILED) {
				perror("map failed");
				exit(1);
			}
			forth_extra_data[child_id].page_table[i] = frame_idx;
			frame_idx++;
			*/
		}	
	}
	// push 0 on child forth stack
	switch_current_to(child_id);
	push_onto_forth_stack(&forth_extra_data[child_id].data,0);


	// push 1 on parent forth stack
	switch_current_to(parent);
	push_onto_forth_stack(&forth_extra_data[parent].data,1);
	return child_id;
}

// segv fault handler from VM1
static void handler(int sig, siginfo_t *si, void *unused)
{
    void* fault_address = si->si_addr;

    //printf("in handler with invalid address %p\n", fault_address);
	
	// calculate the desired page number that caused this segfault
	int page_num = ((void*) fault_address - (void*) UNIVERSAL_PAGE_START) / getpagesize();
	if (page_num < 0 || page_num > NUM_PAGES) {
		printf("address not within expected page!\n");
		exit(2);
	}

	// if segfault was caused by a write on a readonly portion of memory
	int frame_attempt = forth_extra_data[forth_idx].page_table[page_num];
	if (frame_attempt != PAGE_UNCREATED) {
		void* page_data = (void*) frames + (frame_attempt*getpagesize());
		void* desired_addr = (void*) frames + (frame_idx*getpagesize());

		// munmap the page in universal array first
		int munmap_result = munmap((void*) UNIVERSAL_PAGE_START + (page_num*getpagesize()),getpagesize());
		if (munmap_result < 0) {
			perror("munmap failed");
			exit(6);
		} 

		// copy parent page data into a new frame for the child
		void* mem_result = memcpy(desired_addr,page_data,getpagesize());
		if (mem_result != desired_addr) {
			perror("memcpy failed");
			exit(4);
		}
		// map child frame in universal region as writeable
		desired_addr = (void*) UNIVERSAL_PAGE_START + (getpagesize()*page_num);
		mem_result = mmap(desired_addr,
						getpagesize(),
						PROT_READ | PROT_WRITE | PROT_EXEC,
						MAP_SHARED | MAP_FIXED,
						frames_fd,
						frame_idx*getpagesize());
		if (mem_result == MAP_FAILED) {
			perror("map failed");
			exit(1);
		}
		forth_extra_data[forth_idx].page_table[page_num] = frame_idx;
		frame_idx++;
		share_count[frame_attempt]--;
		used_pages_count++;
	} 

	// if segfault was a normal memory access attempt
	else {
		void* desired_addr = (void*) UNIVERSAL_PAGE_START + (getpagesize() * page_num);

		// map to the next available physical frame
		int offset = frame_idx*getpagesize();
		void* mem_result = mmap((void*) desired_addr,
						getpagesize(),
						PROT_READ | PROT_WRITE | PROT_EXEC,
						MAP_SHARED | MAP_FIXED,
						frames_fd,
						offset);
		if(mem_result == MAP_FAILED) {
			perror("map failed");
			exit(1);
		}
		
		// store global frame_idx in local page_table
		forth_extra_data[forth_idx].page_table[page_num] = frame_idx;
		frame_idx++;
		used_pages_count++;
	}
}

bool first_time = true;
void initialize_forths() {
    if(first_time) {

		// initialize shared_count array to 1 for all frames
		for (int i=0; i<NUM_PAGES*MAX_FORTHS; i++) {
			share_count[i] = 1;
		}

        // here's the place for code you only want to run once, like registering
        // our SEGV signal handler
		frames_fd = open("bigmem.dat", O_RDWR | O_CREAT | O_TRUNC, S_IRWXU);
		if (frames_fd < 0) {
			perror("error loading linked file");
			exit(25);
		}
		char data = '\0';
		lseek(frames_fd, getpagesize()*NUM_PAGES*MAX_FORTHS, SEEK_SET);
		write(frames_fd, &data, 1);
		lseek(frames_fd, 0, SEEK_SET);

		frames = mmap(NULL,
				getpagesize()*NUM_PAGES*MAX_FORTHS,
				PROT_READ | PROT_WRITE | PROT_EXEC,
				MAP_SHARED,
				frames_fd,
				0);
	
		if(frames == NULL) {
			perror("first map failed");
			exit(1);
		}

		// setup segv handler from VM1

		// installing SEGV signal handler
		// incidently we must configure signal handling to occur in its own stack
		// otherwise our segv handler will use the regular stack for its data
		// and it might try and unmap the very memory it is using as its stack
		static char stack[SIGSTKSZ];
		stack_t ss = { 
					  .ss_size = SIGSTKSZ,
					  .ss_sp = stack,
		};  
		
		sigaltstack(&ss, NULL);

		struct sigaction sa; 

		// SIGINFO tells sigaction that the handler is expecting extra parameters
		// ONSTACK tells sigaction our signal handler should use the alternate stack
		sa.sa_flags = SA_SIGINFO | SA_ONSTACK;
		sigemptyset(&sa.sa_mask);
		sa.sa_sigaction = handler;
		
		//this is the more modern equalivant of signal, but with a few
		//more options
		if (sigaction(SIGSEGV, &sa, NULL) == -1) {
			perror("error installing handler");
			exit(3);
		}

        first_time = false;
               
    }
    // here's the place for code you want to run every time we run a test case

    // mark all the forths as invalid
    for(int i = 0; i < MAX_FORTHS; i++) {
        forth_extra_data[i].valid = false;
    }
    
    used_pages_count = 0;

}

// This function creates a brand new forth instance (not a fork) with the given code
// The function returns the id num of the newly created forth
int create_forth(char* code) {
    int forth_num = find_available_slot();
    forth_extra_data[forth_num].valid = true;

	// initialize page table 
	for (int i=0; i<NUM_PAGES; i++) {
		forth_extra_data[forth_num].page_table[i] = PAGE_UNCREATED;
	}

    // STEP 0
    // this is where you should allocate NUM_PAGES*getpagesize() bytes
    // starting at position UNIVERSAL_PAGE_START to get started
    //
    // use mmap
	/*
	void* result = mmap((void*) UNIVERSAL_PAGE_START,NUM_PAGES*getpagesize(),
					PROT_READ | PROT_WRITE | PROT_EXEC,
					MAP_FIXED | MAP_SHARED | MAP_ANONYMOUS,
					-1,
					0);
	if(result == MAP_FAILED) {
		perror("map failed");
		exit(1);
	}
	*/
	switch_current_to(forth_num);


    // the return stack is a forth-specific data structure.  I
    // allocate a seperate space for it as the last 2 pages of
    // NUM_PAGES.
    int returnstack_size = getpagesize() * 2;

    int stackheap_size = getpagesize() * (NUM_PAGES - 2);

    // note that in this system, to make forking possible, all forths
    // are created with pointers only in the universal memory region
    initialize_forth_data(&forth_extra_data[forth_num].data,
                          (void*) UNIVERSAL_PAGE_START + stackheap_size + returnstack_size, //beginning of returnstack
                          (void*) UNIVERSAL_PAGE_START, //begining of heap
                          (void*) UNIVERSAL_PAGE_START + stackheap_size); //beginning of the stack


    load_starter_forth(&forth_extra_data[forth_num].data);

    forth_extra_data[forth_num].data.input_current = code;
    return forth_num;
}

struct run_output run_forth_until_event(int forth_to_run) {
    struct run_output output;
	switch_current_to(forth_to_run);
    output.result_code = f_run(&forth_extra_data[forth_to_run].data,
                               NULL,
                               output.output,
                               sizeof(output.output));
    output.forked_child_id = -1; // this should only be set to a value if we are forking
    if(output.result_code == FCONTINUE_FORK) {
        //printf("fork not yet implemented\n");
        output.forked_child_id = fork_forth(forth_to_run);
    } 

    return output;

}
