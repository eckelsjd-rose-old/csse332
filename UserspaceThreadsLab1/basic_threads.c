/*
Basic Threads - a rudimentary userspace threads library

Author: Buffalo (hewner@rose-hulman.edu) and you!

Contrary to C convention (but for your convenience) we've documented
these functions here in the .c file rather than the header.

 */
#include <malloc.h>
#include <ucontext.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "basic_threads.h"

// 64kB stack
#define THREAD_STACK_SIZE 1024*64

/*
max number of threads

note if we had an expandable structure like an arraylist in C it
would be pretty easy to make this arbitrary, but let's not
introduce extra C libraries.

also note that the max we've picked is insanely small.  These threads
are very lightweight so we could probably have thousands.  But that
would make writing test code that exploits the thread max more
annoying.  So please leave this value as it is and use MAX_THREADS
(not the hardcorded value 5) in your code.
*/
#define MAX_THREADS 5

// storage for your thread data
ucontext_t threads[MAX_THREADS]; // array of all available slots for threads
ucontext_t parent; // context scheduler
bool child_done[MAX_THREADS]; // check status of all threads
bool valid_threads[MAX_THREADS]; // check for open slots of threads
int current = 0; // track current thread
int num_threads = 0; // track total number of open threads

// add additional constants and globals here as you need


/*
initialize_basic_threads

A function that resets any globals to a brand new clean state - put
whatever you want in here.

You can rely on the fact that this function is called before any calls
to create_new_thread or schedule_threads.

Note that this function run at the beginning of each test case, so be
sure to use it to ensure your test cases don't leave data that can
effect each other.

BUT as you're starting out there's no need to agonize over what to put
in here.  As you have global data you intend to be initialized before
each test, add the code here.  As you begin, you can just leave this
blank.

 */
void initialize_basic_threads() {
    // printf("Initializing threads\n");
    for (int i = 0; i < MAX_THREADS; i++) {
		child_done[i] = false;
		valid_threads[i] = false;
    }
    num_threads = 0;
    current = 0;
}

/*
create_new_thread

Gets a new thread ready to run, but does not start it.  It will be
started within schedule_threads() when it is this thread's turn (see
below).

This function takes a function pointer to the function the thread
should run when it starts.  The function provided should take no
parameters and return nothing (at least in our first iteration).

To create a new thread, memory must be allocated to store the thread's
stack.  This function should malloc that memory.

The function could fail either because the number of threads is at max
or enough memory cannot be malloc'ed.  Either way, it's fine if this
function prints and exits the program (use "exit(errorCode);" to exit
with an error code - note that 0 means no error)
Example usage:
// elsewhere

create_new_thread(thread_function());

 */
void create_new_thread(void (*fun_ptr)()) {
    create_new_parameterized_thread(fun_ptr, NULL);
}


/*
create_new_parameterized_thread

Don't fill out this function till you get to Test 4.

This function works exactly like create_new_thread, except it expects
a function that takes a void pointer as a paramter, plus a value for
that parameter.

Example Usage:

void takesAnInt(void* val) {
    int* int_ptr = (int*) val;
    //more code
}

//elsewhere

initialize_basic_threads();
int val = 7;
create_new_parameterized_thread(takesAnInt, &val);
schedule_threads();


 */

// helper function to automatically close threads when they return
void make_context(void (*cast_ptr)(), void* parameter) {
    cast_ptr(parameter);
    finish_thread();
}

void create_new_parameterized_thread(void (*fun_ptr)(void*), void* parameter) {
    // cast to (void(*)()); ie a pointer to a function that takes nothing and returns nothing
    void(*cast_ptr)() = (void(*)()) fun_ptr;
    // check if there is a thread location available
    if (num_threads >= MAX_THREADS) {
        perror( "no threads available");
        exit(2);
    }
    // find next available slot to insert new thread
    int insert_location = current;
    bool inserted = false;
    while (!inserted) {
        // found spot to insert a new thread (in an unoccupied position)
        if (!valid_threads[insert_location]) {
            // setup and malloc stack space for the context for the new thread
            getcontext( &threads[insert_location]);
            threads[insert_location].uc_link = 0;
            threads[insert_location].uc_stack.ss_sp = malloc( THREAD_STACK_SIZE );
            threads[insert_location].uc_stack.ss_size = THREAD_STACK_SIZE;
            threads[insert_location].uc_stack.ss_flags = 0;
            if ( threads[insert_location].uc_stack.ss_sp == 0 ) {
            perror( "malloc: Could not allocate stack" );
            exit (1);
            }
            // Create the new context
            // printf( "Creating child thread: %d\n",insert_location);

            // funcptr defined as a pointer to a function that returns nothing and takes in a void, no-arg function pointer and a void pointer
            typedef void(*funcptr)(void(*)(), void*);
            // store the address of the helper make_context function
            funcptr pf = &make_context;
            // cast pf to a pointer to a function that takes no args (even though it does) for the sake of makecontext call
            void(*cast_makecontext)() = (void(*)()) pf;
            // cast_makecontext will be passed 2 parameters: "cast_ptr" and "parameter"
            makecontext( &threads[insert_location], cast_makecontext, 2, cast_ptr, parameter);
            valid_threads[insert_location] = true;
            child_done[insert_location] = false;
            num_threads++;
            inserted = true;
        }
        // iterate in circular fashion around the array of threads
        insert_location = (insert_location + 1) % MAX_THREADS;
    }
}


/*
schedule_threads

This function should be called once all the initial threads have been
created.  The code that calls it becomes the "master" scheduler
thread.

It should switch to one of the newly created threads.  When that
thread yields (see the next required function) control should return
to the scheduler thread.  Then the scheduler should switch to another
available thread.  The scheduler should continue to switch between
threads until every thread has run completely (i.e. not just yielded
once, but actually returned).  Once every other thread has finished,
schedule_threads() should return.

Threads in this assignment should be scheduled in the simplest
scheduling possible - "round robin".  That is, if we have 3 threads we
run 1 2 3 1 2 3 1 2 3 etc. - giving each thread an equal turn.  We'll
talk about why you might want more fancy scheduling systems later in
the course.

Example usage:

create_new_thread(thread_function1());
create_new_thread(thread_function2());
create_new_thread(thread_function3());

printf("Starting threads...");
schedule_threads()
printf("All threads finished");
*/
void schedule_threads() {
    while (num_threads > 0) {
		current = 0;
		for (int j = 0; j < MAX_THREADS; j++) {
			if (valid_threads[j]) {
				current = j;
				if (!child_done[j]) {
					swapcontext( &parent, &threads[j]);
				} else {
					num_threads--;
                    valid_threads[current] = false;
                    free(threads[current].uc_stack.ss_sp);
				}
			}
		}
    }
}

/*
yield

This function is called within a thread to indicate that it is ready
to allow things to switch and other threads to run (for a time).  The
threading we will write for this assignment will be non-preemptive:
threads will have to manually give control back to the scheduler by
calling yield.

Yield should use swapcontext to put the scheduler back in control and
save the current state in an appropriate u_context variable.  Later,
when the scheduler opts to run the yielding threat again swapcontext
will appear to have returned normally, the yield function itself can
return, and execution continues normally.

Note: The fact that this threading system is non-preemptive means that
when a thread is within a long-running calculation the programmer must
remember to periodically call yield or the system will appear to lock
up.  We'll handle adding preemption in a future assignment, but it
will be complicated.

Example usage:

void thread_function()
{
    for(int i = 0; i < 200; i++) {
        printf( "working\n" );
        
        // allow other threads to do some work too
        yield();
        // ok, switched back, better do some more work
    }
    printf( "done\n" );
    
    // like yield but never switches back
    finish_thread();
}

*/
void yield() {
    // printf("Pausing thread: %d\n", current);
    swapcontext( &threads[current], &parent );
}

/*
finish_thread

This function works like yield but also marks things so that the
thread is marked as finished and won't be scheduled again.

Eventually, we'll figure out a way to have this function called
implicitly when the thread function returns but for simplicity in our
earily examples we just call it directly.

Note: This is not a good place to call the free corresponding to the
malloc in create_new_thread.  For the first couple tests it's OK to
let that memory leak and then we'll discuss the issue in detail.

Example usage:

void thread_function()
{
    printf("thread running\n");
    finish_thread();
    printf("If this lines prints, finish thread is broken\n");
}

*/
void finish_thread() {
    // printf( "Child thread %d exiting\n", current );
    child_done[current] = true;
    swapcontext( &threads[current], &parent );
}
