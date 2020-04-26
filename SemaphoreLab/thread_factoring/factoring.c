/* Copyright 2016 Rose-Hulman Institute of Technology

Here is some code that factors in a super dumb way.  We won't be
attempting to improve the algorithm in this case (though that would be
the correct thing to do).

Modify the code so that it starts the specified number of threads and
splits the computation among them.  You can be sure the max allowed 
number of threads is 50.  Be sure your threads actually run in parallel.

Your threads should each just print the factors they find, they don't
need to communicate the factors to the original thread.

ALSO - be sure to compile this code with -pthread or just used the 
Makefile provided.

 */
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <pthread.h>
#include <stdlib.h>

// global variables used by threads (these do not change)
unsigned long long int target;
int numThreads;
int workSize;

// struct to pass multiple arguments to a thread function
struct arg_struct {
	int start_num;
	int thread_num;
};

// thread function to find factors
// find_factors should be passed an arg_struct with the thread number and starting number
// the starting number is where this thread will begin looking for factors
// each thread should look for 'workSize' number of factors of the 'target' number
void *find_factors(void* ptr) {
	struct arg_struct *args = (struct arg_struct*) ptr;
	for (unsigned long long int i = args->start_num; i < args->start_num + workSize; i++) {
		printf("thread %d testing %llu\n", args->thread_num, i);
		if (target % i == 0) {
			printf("%llu is a factor\n", i);
		}
	}
	pthread_exit(NULL);
}

int main(void) {
    /* you can ignore the linter warning about this */
    unsigned long long int i = 0;
	int rtn;
	pthread_attr_t attr;
    printf("Give a number to factor.\n");
    scanf("%llu", &target);

    printf("How many threads should I create?\n");
    scanf("%d", &numThreads);
    if (numThreads > 50 || numThreads < 1) {
      printf("Bad number of threads!\n");
      return 0;
    }
	int starting_num[numThreads]; // store thread starting numbers
	struct arg_struct args[numThreads]; // array of arg structs to pass to threads
	pthread_t tid[numThreads];    // store thread id numbers
	workSize = target/numThreads; // equal to equivalent workload shared by each thread

	pthread_attr_init(&attr);
	rtn = pthread_attr_setscope(&attr, PTHREAD_SCOPE_PROCESS);
	printf("setscope: %d\n",rtn);

	// create 'numThreads' threads, starting at 1 to avoid 0 as a factor
	for (i = 1; i <= numThreads; i++) {
		starting_num[i-1] = workSize*(i-1) + 1;
		args[i-1].start_num = starting_num[i-1];
		args[i-1].thread_num = i;
		pthread_create(&tid[i-1], &attr, find_factors, (void *) &args[i-1]);
	}

	// wait for all threads to finish
	for (i = 0; i < numThreads; i++) {
		pthread_join(tid[i], NULL);
	}

	printf("All threads complete\n");
	return 0;
		
 // for (i = 2; i <= target/2; i = i + 1) {
 //   /* You'll want to keep this testing line in.  Otherwise it goes so
 //      fast it can be hard to detect your code is running in
 //      parallel. Also test with a large number (i.e. > 3000) */
 //   printf("testing %llu\n", i);
 //   if (target % i == 0) {
 //     printf("%llu is a factor\n", i);
 //   }
 // }
    return 0;
}

