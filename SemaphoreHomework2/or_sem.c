#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdlib.h>

/**
    The goal of sem_or is to create a new type of waiting that will
    wait on two semaphores at once, and then return when either
    semaphore is unlocked.

    The basic implementation of the idea is to create two new threads,
    each of which will wait on one of the two semaphores.  Then the
    original thread waits on a newly created semaphore.  When one of
    the newly created thread wakes up, it sees if it is the first one
    to awaken, and if so it posts on the newly created semaphore
    allowing the original thread to continue.

    This is more complex that it seems for three reasons:

    1. the two semaphores can be unlocked at almost the same time, so
    we must ensure that only 1 semaphore can be considered first.  The
    sem_wait_or function also returns a pointer to the semaphore that
    finished first - we must ensure that the second semaphore thread
    does not dispose of the relevant resources before that value is
    safely returned.

    2. Because we want to be able to call sem_wait_or as much as we
    wish, all the data and semaphores associated must be allocated on
    the heap (e.g. with malloc).  

    3. It is not safe to free the heap data when the first semaphore
    returns.  There is a second thread, waiting on the 2nd semaphore
    that requires resources to run.  For that reason, the sem_wait_or
    process is not complete until both of the two branches of the or
    have recieved a post.  This is a natural consequence of the fact
    that there is no safe way to remove a thread from a semaphore
    queue, once it is waiting.

    In the second branch, the resources of the sem_wait_or must be
    fully cleaned up.

    This includes (NOT necessarily in order):

        a. ensuring that the first thread has returned the pointer to
        the first semaphore to the main thread

        b. doing a sem_destroy on all semaphores created in heap
        memory at the beginning of the process

        c. ensuring that a thread_join has been done on the first
        thread (either here or elsewhere) to free its resources

        d. ensuring that a pthread_detach has been done on the 2nd
        thread (check out the main page, but this in essence frees the
        resources of a thread that will not be pthread_joined).

        e. freeing all resources malloced in the sem_wait_or process

        f. doing a post on the 2nd semaphore - this last one is
        debatable but the basic idea is that if we doing something
        like this:

        sem_wait_or(&sema, &semb)
        sem_wait_or(&sema, &semb)
        
        two posts to either semaphore OR a combination should unlock
        the process.  Put another way, the sem_or only needs to be
        unlocked once - once it has been, the other thread "passes"
        any post it gets back into the semaphore as if it was not
        really in the queue.

    We will use valgrind to ensure your system does not leak
    resources.

    Example output:
    got y
    or called
    or called
    freeing y
    got y
    freeing y
    or wait returned
    or wait got y (0x5582829a8060)
    got x
    freeing x
    got x
    joining 0
    freeing x
    or wait returned
    joining 1
    joining 2
    joining 3
    joining 4
    Everything finished.

**/
struct all_data {
    // store whatever you need to get this working
    // just be sure you clean up
	sem_t *sem1;
	sem_t *sem2;
	sem_t *main_sem;
	sem_t *mutex; // protects the 'first' int argument
	int first; // decides if a thread was the first to wake up
	sem_t *ret; // signals successful return
	sem_t *retval;
	pthread_t thread1;
	pthread_t thread2;
};

// thread 1 will wait on sem 1
void* func1(void* args) {
	struct all_data *data = (struct all_data*) args;
	sem_wait(data->sem1);

	sem_wait(data->mutex); // achieve lock
	// if first thread
	if (data->first == 0) {
		data->first++;
		sem_post(data->mutex); // unlock for other thread
		data->retval = data->sem1; // set retval
		sem_post(data->main_sem); // tell main thread to continue
		sem_post(data->ret); // tell second thread that return value was successful
		pthread_exit(NULL);
	} else { // if second thread
		sem_wait(data->ret); // ensure retval of 1st thread
		sem_post(data->sem2);
		pthread_join(data->thread2,NULL); // join thread 2
		
		sem_destroy(data->mutex);
		sem_destroy(data->ret);
		sem_destroy(data->retval);

		pthread_detach(data->thread1);
		free(data);
		return NULL;
	}
}

// thread 2 will wait on sem 2
void* func2(void* args) {
	struct all_data *data = (struct all_data*) args;
	sem_wait(data->sem2);

	sem_wait(data->mutex);
	// if first thread
	if (data->first == 0) {
		data->first++;
		sem_post(data->mutex);
		data->retval = data->sem2; // set retval
		sem_post(data->main_sem);
		sem_post(data->ret);
		pthread_exit(NULL);
	} else { // if second thread
		sem_wait(data->ret); // ensure retval of 1st thread
		sem_post(data->sem1);
		pthread_join(data->thread1,NULL); // join thread 1
	
		sem_destroy(data->mutex);
		sem_destroy(data->ret);
		sem_destroy(data->retval);

		pthread_detach(data->thread2);
		free(data);
		return NULL;
	}
}
    
sem_t* sem_wait_or(sem_t* a, sem_t* b) {
    // Implement me
	pthread_t thread1;
	pthread_t thread2;
	sem_t main_thread;
	sem_t mutex;
	sem_t ret;
	sem_init(&main_thread,0,0);
	sem_init(&mutex,0,1);
	sem_init(&ret,0,0);

	// create args to pass to pthreads
	struct all_data *args = malloc(sizeof(struct all_data));
	args->sem1 = a;
	args->sem2 = b;
	args->main_sem = &main_thread; 
	args->first = 0;
	args->mutex = &mutex;
	args->ret = &ret;
	args->thread1 = thread1;
	args->thread2 = thread2;
	
	pthread_create(&thread1,NULL,func1,(void *) args); // start waiting for two sems in parallel
	pthread_create(&thread2,NULL,func2,(void *) args);
	sem_wait(&main_thread); // wait for one of the sems to run

	// the first thread will set retval and immediately return here
	// second thread is responsible for pthread_joining it and then detaching
	sem_post(args->retval);
	sem_destroy(&main_thread);
	return args->retval;
	/*
	void *status1;
	void *status2;
	pthread_join(thread1,&status1);
	pthread_join(thread2,&status2);

	void *status = (status1 == NULL) ? status2 : status1; // get the right return value
	free(args);

	sem_destroy(&main_thread);
	sem_destroy(&mutex);
	sem_destroy(&ret);
	return (sem_t *) status;
	*/
}

sem_t x, y;

void* xfunc(void* ignored) {
    sem_wait(&x);
    printf("got x\n");
    sleep(1);
    printf("freeing x\n");
    sem_post(&x);
    return NULL;
}

void* yfunc(void* ignored) {
    sem_wait(&y);
    printf("got y\n");
    sleep(1);
    printf("freeing y\n");
    sem_post(&y);
    return NULL;
}


void* my_or(void* ignored) {
    sem_t *result = sem_wait_or(&x, &y);
    printf("or wait got %s (%p)\n", result == &x ? "x" : "y", result);
    if(result != &x && result != &y) {
        printf("ERROR or returned a bogus result\n");
        return NULL;
    }
    if(result == &x) sem_post(&y);
    if(result == &y) sem_post(&x);
    return NULL;

}

int main(int argc, char **argv) {
    pthread_t threads[100];
    int cur_thread = 0;

    // to see the or behavior, only leave one of these unlocked
    sem_init(&x, 0, 0);
    sem_init(&y, 0, 1);

    pthread_create(&threads[cur_thread], NULL, xfunc, NULL);
    cur_thread++;
    pthread_create(&threads[cur_thread], NULL, xfunc, NULL);
    cur_thread++;
    pthread_create(&threads[cur_thread], NULL, yfunc, NULL);
    cur_thread++;
    pthread_create(&threads[cur_thread], NULL, yfunc, NULL);
    cur_thread++;

    // ensure all of the threads of one type have finished
    sleep(1);
    
    // now the my_or will have to use one of these semaphores but not
    // the other.  However, having used one, it unlocks the other so
    // the other side can finish its threads too.
    pthread_create(&threads[cur_thread], NULL, my_or, NULL);
    cur_thread++;

    //sleep(4);
    for(int i = 0; i < cur_thread; i++) {

        pthread_join(threads[i], NULL);
        printf("joining %d\n", i);
    }
    printf("Everything finished.\n");
    
}
