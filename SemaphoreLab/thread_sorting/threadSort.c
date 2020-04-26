#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/time.h>
#include <pthread.h>
#include <string.h>

// max number of its in input file
#define MAX_BUFFER_SIZE 10000
// max threads allowed
#define MAX_N_SIZE 100


/* other global variable instantiations can go here */
int threads[MAX_N_SIZE][MAX_BUFFER_SIZE];
double runtime[3][MAX_BUFFER_SIZE]; // track runtime of each of the three sorts
int brute_idx = 0; // track indices for the runtime 2D array
int bubble_idx = 0;
int merge_idx = 0;


// struct to pass multiple arguments in to thread function
struct arg_struct {
	int thread_num;
	int sort_type; // 0=BruteForce, 1=BubbleSort, 2=MergeSort
	double diff;
	long secs_used;
	long usecs_used;
	struct timeval start;
	struct timeval end;
};

// helper function to get number of non-zero elements in array
// type=0 -> int[], type=1 -> float[]
int length(void* array, int type) {
	int count = 0;
	int i;
	if (type == 0) { 
		int* int_array = (int*) array;
		for (i = 0; i < MAX_BUFFER_SIZE; i++) {
			if (int_array[i] != 0) { count++;}
		}
	}
    else if (type == 1) { 
		double* float_array = (double*) array;
		for (i = 0; i < MAX_BUFFER_SIZE; i++) {
			if (float_array[i] != 0) { count++;}
		}
	}
	return count;
}

/* Uses a brute force method of sorting the input list. */
void BruteForceSort(int inputList[], int inputLength) {
  int i, j, temp;
  for (i = 0; i < inputLength; i++) {
    for (j = i+1; j < inputLength; j++) {
      if (inputList[j] < inputList[i]) {
        temp = inputList[j];
        inputList[j] = inputList[i];
        inputList[i] = temp;
      }
    }
  }
}

/* Uses the bubble sort method of sorting the input list. */
void BubbleSort(int inputList[], int inputLength) {
  char sorted = 0;
  int i, temp;
  while (!sorted) {
    sorted = 1;
    for (i = 1; i < inputLength; i++) {
      if (inputList[i] < inputList[i-1]) {
        sorted = 0;
        temp = inputList[i-1];
        inputList[i-1] = inputList[i];
        inputList[i] = temp;
      }
    }
  }
}

/* Merges two arrays.  Instead of having two arrays as input, it
 * merges positions in the overall array by re-ording data.  This 
 * saves space. */
void Merge(int *array, int left, int mid, int right) {
  int tempArray[MAX_BUFFER_SIZE];
  int pos = 0, lpos = left, rpos = mid + 1;
  while (lpos <= mid && rpos <= right) {
    if (array[lpos] < array[rpos]) {
      tempArray[pos++] = array[lpos++];
    } else {
      tempArray[pos++] = array[rpos++];
    }
  }
  while (lpos <= mid)  tempArray[pos++] = array[lpos++];
  while (rpos <= right)tempArray[pos++] = array[rpos++];
  int iter;
  for (iter = 0; iter < pos; iter++) {
    array[iter+left] = tempArray[iter];
  }
  return;
}

/* Divides an array into halfs to merge together in order. */
void MergeSort(int *array, int left, int right) {
  int mid = (left+right)/2;
  if (left < right) {
    MergeSort(array, left, mid);
    MergeSort(array, mid+1, right);
    Merge(array, left, mid, right);
  }
}

// thread function
void *sort(void* ptr) {
	struct arg_struct *args = (struct arg_struct*) ptr;
	struct timeval start = args->start;
	struct timeval end = args->end;
	double diff = args->diff;
	long secs_used = args->secs_used;
	long usecs_used = args->usecs_used;
	
	// sort and save runtimes
	switch (args->sort_type)
	{
		case 0: // BruteForce
			gettimeofday(&start,NULL);
			BruteForceSort(threads[args->thread_num],length(threads[args->thread_num],0));
			gettimeofday(&end,NULL);
			secs_used=(end.tv_sec - start.tv_sec);
			usecs_used=(end.tv_usec - start.tv_usec);
			diff = secs_used + (double) usecs_used/1000000;
			runtime[0][brute_idx++] = diff;
			break;
		case 1: // BubbleSort
			gettimeofday(&start,NULL);
			BubbleSort(threads[args->thread_num],length(threads[args->thread_num],0));
			gettimeofday(&end,NULL);	
			secs_used=(end.tv_sec - start.tv_sec);
			usecs_used=(end.tv_usec - start.tv_usec);
			diff = secs_used + (double) usecs_used/1000000;
			runtime[1][bubble_idx++] = diff;
			break;
		case 2: // MergeSort
			gettimeofday(&start,NULL);
			MergeSort(threads[args->thread_num],0, length(threads[args->thread_num]-1,0));
			gettimeofday(&end,NULL);	
			secs_used=(end.tv_sec - start.tv_sec);
			usecs_used=(end.tv_usec - start.tv_usec);
			diff = secs_used + (double) usecs_used/1000000;
			runtime[2][merge_idx++] = diff;
			break;
		default:
			printf("No such search method. Error occurred.\n");
			exit(1);
	}
	printf("Thread %d took %f sec\n",args->thread_num,diff);
	pthread_exit(NULL);
}

void print_runtime_info() {
	double stats[3][3]; // col 0=min, 1=max, 2=mean
					 // row 0=brute, 1=bubble, 2=merge
	const char* names[] = {"Brute force", "BubbleSort", "MergeSort"};
	double sum;
	for (int i = 0; i < 3; i++) {
		stats[i][0] = runtime[i][0]; // initialize min
		stats[i][0] = runtime[i][0]; // initialize max
		sum = 0;
		for (int j = 0; j < length(runtime[i],1); j++) {
			sum += runtime[i][j];
			if (runtime[i][j] > stats[i][1]) {
				stats[i][1] = runtime[i][j];
			}
			if (runtime[i][j] < stats[i][0]) {
				stats[i][0] = runtime[i][j];
			}
		}
		stats[i][2] = sum / length(runtime[i],1);
		printf("%s. Min: %8.6f. Max: %8.6f. Mean: %8.6f.\n",names[i],stats[i][0],stats[i][1],stats[i][2]);
	}
}

int main(int argc, char** argv) {

    if(argc < 4) {
        printf("not enough arguments!\n");
        exit(1);
    }
	memset(threads, 0, sizeof(threads));
	memset(runtime, 0, sizeof(runtime));

    // I'm reading the value n (number of threads) for you off the
    // command line
    int numThreads = atoi(argv[1]);
    if(numThreads <= 0 || numThreads > MAX_N_SIZE) {
        printf("bad n value (number of threads) %d\n", numThreads);
        exit(1);
    }
    
    int read_fd = open(argv[2], O_RDONLY);
    if(read_fd == -1) {
        perror("couldn't open file for reading");
        exit(1);
    }

    char buffer[6]; //we're reading ints, so this is plenty for a line
    int read_result;
	int j = 0; // will track row number (represents each thread)
	int k = 0; // will track column number
	int i = 0;
    while((read_result = read(read_fd, buffer, 6))) {
        int data = atoi(buffer);
        // this is where you'll divvy up the integers into buffers for
        // the various threads
		threads[j][k] = data;
		if (j+1 == numThreads) { k++; } // fill vertically by column (evenly distribute across threads)
		j = (j+1) % numThreads;
    }
    if(read_result < 0) {
        perror("file read error");
        exit(1);
    } else {
        close(read_fd);
    }
    // this is where you'll dispatch threads for sorting
    // then wait for things to finish
    // then print the statistics
	int rtn;
	pthread_attr_t attr;
	struct arg_struct args[numThreads]; // array of arg structs
	pthread_t tid[numThreads];			// array of thread id numbers
	pthread_attr_init(&attr);
	rtn = pthread_attr_setscope(&attr, PTHREAD_SCOPE_PROCESS);
	printf("setscope: %d\n", rtn);

	// create threads, one for each row in threads[][] 2D array
	k = 0;
	for (i = 0; i < numThreads; i++) {
		args[i].thread_num = i;
		args[i].sort_type = k;
		k = (k+1) % 3;
		pthread_create(&tid[i], &attr, sort, (void *) &args[i]);
	}

	// wait for all threads to finish
	for (i = 0; i < numThreads; i++) {
		pthread_join(tid[i], NULL);
	}

	printf("All threads complete\n");
	print_runtime_info();

    // make one giant buffer capable of storing all your
    // output.
	// each row i of (int** threads) is sorted horizontally, so copy data into the big buffer by row
	k = 0;
	int mid = 0;
	int big_array[MAX_BUFFER_SIZE];
	for (i=0; i < numThreads; i++) {
		for (j=0; j<length(threads[i],0); j++) {
			big_array[k++] = threads[i][j];
		}
		mid += length(threads[i],0);
		Merge(big_array,0,mid,k-1); // move a new row in and merge with current
		// 'k' will mark the next available index in big_array[k] always
		// as such 'k-1' is the right most point and k/2 is the midpoint
	}

    // make a loop to copy data into it, then use the Merge function
    // to merge that data, then copy more data into it etc.

    // once everything is sorted, write it out to the file

    int output_fd;
    if ((output_fd = open(argv[3], O_WRONLY | O_CREAT | O_TRUNC,
                    S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)) == -1) {
        perror("Cannot open output file\n");
        exit(1);
    }
	for (k=0;k<MAX_BUFFER_SIZE;k++) {
    	dprintf(output_fd, "%d\n", big_array[k]);
	}
    close(output_fd);

}
