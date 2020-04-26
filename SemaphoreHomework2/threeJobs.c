/* Copyright 2019 Rose-Hulman */
#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>


// number of carpenters
#define NUM_CARP 3
// number of painters
#define NUM_PAIN 3
// number of decorators
#define NUM_DECO 3

int carp_count = 0;
int pain_count = 0;
int deco_count = 0;

sem_t carp_mutex;
sem_t pain_mutex;
sem_t deco_mutex;

sem_t carp_block;
sem_t pain_block;
sem_t deco_block;

/**
   Imagine there is a shared memory space called house.

   There are 3 different kinds of operations on house: carpenters,
   painters, and decorators.  For any particular kind of operation,
   there can be an unlimited number of threads doing the same
   operation at once (e.g. unlimited carpenter threads etc.).
   However, only one kind of operation can be done at a time (so even
   a single carpenter should block all painters and vice versa).

   Use semaphores to enforce this constraint.  You don't have to worry
   about starvation (e.g. that constantly arriving decorators might
   prevent carpenters from ever running) - though maybe it would be
   fun to consider how you would solve in that case.

   This is similar to the readers/writers problem BTW.
**/

void* carpenter(void * ignored) {
	// obtain mutex to modify carp_count
	sem_wait(&carp_mutex);
	carp_count++;
	if (carp_count == 1) {
		sem_wait(&pain_block); // first carp blocks all decos and pains
		sem_wait(&deco_block);
	}
	sem_post(&carp_mutex);

	// critical section
    printf("starting carpentry\n");
    sleep(1);
    printf("finished carpentry\n");

	// modify carp_count
	sem_wait(&carp_mutex);
	carp_count--;
	if (carp_count == 0) {
		sem_post(&pain_block); // last carp releases blocks
		sem_post(&deco_block);
	}
	sem_post(&carp_mutex);

    return NULL;
}

void* painter(void * ignored) {
	// obtain mutex to modify pain_count
	sem_wait(&pain_mutex);
	pain_count++;
	if (pain_count == 1) {
		sem_wait(&carp_block); // block all carps and decos
		sem_wait(&deco_block);
	}
	sem_post(&pain_mutex);

	// critical section
    printf("starting painting\n");
    sleep(1);
    printf("finished painting\n");

	// modify pain count
	sem_wait(&pain_mutex);
	pain_count--;
	if (pain_count == 0) {
		sem_post(&carp_block); // release blocks
		sem_post(&deco_block);
	}
	sem_post(&pain_mutex);

    return NULL;
}

void* decorator(void * ignored) {
	// obtain mutex to modify deco_count
	sem_wait(&deco_mutex);
	deco_count++;
	if (deco_count == 1) {
		sem_wait(&carp_block); // block all carps and pains
		sem_wait(&pain_block);
	}
	sem_post(&deco_mutex);

	// critical section
    printf("starting decorating\n");
    sleep(1);
    printf("finished decorating\n");
	
	// modify deco_count
	sem_wait(&deco_mutex);
	deco_count--;
	if (deco_count == 0) {
		sem_post(&carp_block); // release all blocks
		sem_post(&pain_block);
	}
	sem_post(&deco_mutex);

    return NULL;
}


int main(int argc, char **argv) {
  pthread_t jobs[NUM_CARP + NUM_PAIN + NUM_DECO];
	sem_init(&carp_mutex,0,1);
	sem_init(&pain_mutex,0,1);
	sem_init(&deco_mutex,0,1);
	sem_init(&carp_block,0,1);
	sem_init(&pain_block,0,1);
	sem_init(&deco_block,0,1);
    for (int i = 0; i < NUM_CARP + NUM_PAIN + NUM_DECO; i++) {
        void* (*func) (void*) = NULL;
        if(i < NUM_CARP)
            func = carpenter;
        if(i >= NUM_CARP && i < NUM_CARP + NUM_PAIN)
            func = painter;
        if(i >= NUM_CARP + NUM_PAIN) {
            func = decorator;
        }
        pthread_create(&jobs[i], NULL, func, NULL);
    }

    for (int i = 0; i < NUM_CARP + NUM_PAIN + NUM_DECO; i++) {
        pthread_join(jobs[i], NULL);
    }

    printf("Everything finished.\n");
	sem_destroy(&carp_mutex);
	sem_destroy(&pain_mutex);
	sem_destroy(&deco_mutex);
	sem_destroy(&carp_block);
	sem_destroy(&pain_block);
	sem_destroy(&deco_block);

}
