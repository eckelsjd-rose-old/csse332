/* Copyright 2016 Rose-Hulman */
#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <semaphore.h>

#define NUM_THREADS 10
/*
 * You can use the provided makefile to compile your code.
*/
int total;
sem_t semaphore;

void *add10000(void *arg) {
    for (int i = 0; i < 10000; i++) {
		sem_wait(&semaphore);
        total++;
		sem_post(&semaphore);
    }
    return NULL;
}

int main(int argc, char **argv) {
    total = 0;
	sem_init(&semaphore, 0, 1);
    pthread_t threads[NUM_THREADS];

    int i;
    for (i = 0; i < NUM_THREADS; i++) {
        pthread_create(&threads[i], NULL, add10000, NULL);
    }

    for (i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }
	sem_destroy(&semaphore);

    printf("Everything finished.  Final total %d\n", total);

    return 0;
}
