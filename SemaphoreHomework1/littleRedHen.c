/* Copyright 2016 Rose-Hulman */
#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <stdlib.h>

#define NUM_LOAVES_PER_BATCH 7
#define NUM_BATCHES 6
#define NUM_LOAVES_TO_EAT 14

/**
   This system has four threads: the duck, cat, and dog that eat 
   bread, and the little red hen that makes the bread. The little
   red hen makes seven loaves per batch, but she only has the patience
   for six batches. The little red hen only makes a batch if there are
   no loaves left.

   The other three animals each want to eat 14 loaves of
   bread, but only one of them can be in the kitchen at a time (to avoid
   fights over who gets what bread).

   When the duck, cat, or dog notices that there are no loaves of bread
   available, they complain to the little red hen and wait (in the kitchen)
   for the next batch to be ready.

   Use semaphores to enforce this constraint. Note: the global numLoaves 
   variable should be left as is (i.e. do not make it a semaphore).

   look at littleRedHenSampleOutput.txt for an example correct output
   sequence

**/

int numLoaves;

sem_t hen_sem;
sem_t kitchen;
sem_t batch_ready;

void *littleRedHenThread(void *arg) {
  char *name = (char*)arg;
  int batch;

  for (batch = 1; batch <= 6; batch++) {
    sleep(2);  // just makes it obvious that it won't work without
               // semaphores
	sem_wait(&hen_sem);
    numLoaves += 7;
    printf("%-20s: A fresh batch of bread is ready.\n", name);
    sem_post(&batch_ready);
  }

  printf("%-20s: I'm fed up with feeding you lazy animals! "
  "No more bread!\n", name);
  return NULL;
}

void *otherAnimalThread(void *arg) {
  char *name = (char*)arg;
  int numLoavesEaten = 0;
  while (numLoavesEaten < NUM_LOAVES_TO_EAT) {
	sem_wait(&kitchen);
    if (numLoaves <= 0) {
      printf("%-20s: Hey, Little Red Hen, make some more bread!\n", name);
	  sem_post(&hen_sem);
	  sem_wait(&batch_ready);
    }
    numLoaves--;
    printf("%-20s: Mmm, this loaf is delicious.\n", name);
    numLoavesEaten++;
	sem_post(&kitchen);
    if (random() > random()) {  // Adds variety to output
      sleep(1);
    }
  }
  printf("%-20s: I've had my fill of bread. Thanks, Little Red Hen!\n", name);
  return NULL;
}

int main(int argc, char **argv) {
  pthread_t dog, cat, duck, hen;
  numLoaves = 0;
  char dogName[] = "Lazy Dog";
  char catName[] = "Sleepy Cat";
  char duckName[] = "Noisy Yellow Duck";
  char henName[] = "Little Red Hen";
  sem_init(&hen_sem,0,0);
  sem_init(&kitchen,0,1);
  sem_init(&batch_ready,0,0);

  pthread_create(&dog, NULL, otherAnimalThread, dogName);
  pthread_create(&cat, NULL, otherAnimalThread, catName);
  pthread_create(&duck, NULL, otherAnimalThread, duckName);
  pthread_create(&hen, NULL, littleRedHenThread, henName);
  pthread_join(dog, NULL);
  pthread_join(cat, NULL);
  pthread_join(duck, NULL);
  pthread_join(hen, NULL);

  printf("Everything finished.\n");
  sem_destroy(&hen_sem);
  sem_destroy(&kitchen);
  sem_destroy(&batch_ready);
}
