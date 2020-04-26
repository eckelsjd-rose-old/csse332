/* Copyright 2016 Rose-Hulman */
#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>

/**
   In this system, there are 2 threads.  Thread one prints out A and
   C.  Thread two prints out B and D.  Use semaphores to ensure they
   always print A B C D, in that order.

   Use semaphores to enforce this constraint.
**/

sem_t a_sem;
sem_t b_sem;
sem_t c_sem;
sem_t d_sem;

void *threadOne(void *arg) {
  sleep(1);  /* just makes it obvious that it won't work without
                semaphores */
  sem_wait(&a_sem);
  printf("A\n");
  sem_post(&b_sem);
  sem_wait(&c_sem);
  printf("C\n");
  sem_post(&d_sem);
  return NULL;
}

void *threadTwo(void *arg) {
  sem_wait(&b_sem);
  printf("B\n");
  sem_post(&c_sem);
  sem_wait(&d_sem);
  printf("D\n");
  sem_post(&a_sem);
  return NULL;
}

int main(int argc, char **argv) {
  pthread_t one, two;
  sem_init(&a_sem, 0, 1);
  sem_init(&b_sem, 0, 0);
  sem_init(&c_sem, 0, 0);
  sem_init(&d_sem, 0, 0);

  pthread_create(&one, NULL, threadOne, NULL);
  pthread_create(&two, NULL, threadTwo, NULL);
  pthread_join(one, NULL);
  pthread_join(two, NULL);

  sem_destroy(&a_sem);
  sem_destroy(&b_sem);
  sem_destroy(&c_sem);
  sem_destroy(&d_sem);
  printf("Everything finished.\n");
}
