/* Copyright 2016 Rose-Hulman */
#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>

/**
   In this system, there are 2 types of threads - producers and
   consumers.  Producers write integers to the shared data buffer.
   Consumers read integers from the shared data buffer.  Everybody has
   to be careful when modifying the buffer and the integer that keeps track
   of how many elements are in the buffer.

   When the producers are being slow (i.e. the buffer is empty) the
   consumers should be blocked by a semaphore.  You can simulate this
   by uncommenting the sleep in the producer code.

   When the consumers are being slow (i.e. the buffer is full) the
   producers should be blocked by a semaphore.  You can simulate this
   by uncommenting the sleep in the consumer code.

   It does not matter the order the items are consumed (i.e. it does
   not need to be the same as the order they were produced).
   
   Here's what I would suggest, attempt to figure it out and make a
   first stab at writing the code.  Then, if you get stuck - check out
   the treatmeant in the little book of semaphores (pg 73):

   http://www.greenteapress.com/semaphores/
   
**/

sem_t empty;
sem_t full;
sem_t lock;

#define BUFFERSIZE 5
int buffer[BUFFERSIZE];
int lastValidIndex;

void *producer(void *arg) {
  /* you can ignore this linter error */
  int i, value = *((int*) arg);
  /* I produce 10 values, then I stop */
  for (i = 0; i < 10; i++) {
	sem_wait(&empty);
	sem_wait(&lock);
    buffer[lastValidIndex + 1] = value;
    lastValidIndex++;
    printf("Produced value %d, stored at %d\n", value, lastValidIndex);
    value = value + 1;
	sem_post(&full);
	sem_post(&lock);
	//sleep(1);
  }
  return NULL;
}

void *consumer(void *arg) {
  int value, i;
  /* I consume 10 values, then I stop */
  for (i = 0; i < 10; i++) {
	sem_wait(&full);
	sem_wait(&lock);
    value = buffer[lastValidIndex];
    lastValidIndex--;
    printf("Consumed value %d, stored at %d\n", value, lastValidIndex + 1);
	sem_post(&empty);
	sem_post(&lock);
    //sleep(1);
  }
  return NULL;
}

int main(int argc, char **argv) {
  pthread_t p1, p2, c1, c2;
  int p1start = 100;
  int p2start = 200;
  sem_init(&empty,0,BUFFERSIZE);
  sem_init(&full,0,0);
  sem_init(&lock,0,1);

  lastValidIndex = -1;  /* initially there is no valid data */
  pthread_create(&p1, NULL, producer, &p1start);
  pthread_create(&p2, NULL, producer, &p2start);
  pthread_create(&c1, NULL, consumer, NULL);
  pthread_create(&c2, NULL, consumer, NULL);

  pthread_join(p1, NULL);
  pthread_join(p2, NULL);
  pthread_join(c1, NULL);
  pthread_join(c2, NULL);

  sem_destroy(&empty);
  sem_destroy(&full);
  sem_destroy(&lock);
  printf("Everything finished.\n");
}
