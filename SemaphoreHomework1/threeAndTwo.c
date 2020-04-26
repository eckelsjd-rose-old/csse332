/* Copyright 2016 Rose-Hulman */
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <semaphore.h>

/**
   In this system, there are 2 types of threads - A and B.  Threads 
   A and B can safely run at the same time - they don't affect each
   other.  Only 2 As can safely run at the same time.  Only 3 Bs can
   safely run at the same time.

   Use semaphores to enforce this constraint.
**/
sem_t A_sem;
sem_t B_sem;

void *typeA(void *arg) {
  sem_wait(&A_sem);
  printf("Starting A\n");
  sleep(1);
  printf("A finished.\n");
  sem_post(&A_sem);
  return NULL;
}

void *typeB(void *arg) {
  sem_wait(&B_sem);
  printf("Starting B\n");
  sleep(1);
  printf("B finished.\n");
  sem_post(&B_sem);
  return NULL;
}

int main(int argc, char **argv) {
  pthread_t tid[10];
  pthread_attr_t attr;
  sem_init(&A_sem,0, 2);
  sem_init(&B_sem,0, 3);
  int i;

  for (i = 0; i < 5; i++) {
    pthread_create(&tid[i], NULL, typeA, NULL);
  }
  for (i = 5; i < 10; i++) {
    pthread_create(&tid[i], NULL, typeB, NULL);
  }
  for (i = 0; i < 10; i++) {
    pthread_join(tid[i], NULL);
  }
  printf("Everything finished.\n");
  sem_destroy(&A_sem);
  sem_destroy(&B_sem);
}
