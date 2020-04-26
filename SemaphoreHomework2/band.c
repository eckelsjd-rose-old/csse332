/* Copyright 2019 Rose-Hulman */
#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdlib.h>

/**
   Imagine a group of friends are getting together to play music, but
   they are arriving at different times.  Arriving can happen at any
   time (e.g. when some other friends are playing).

   There are 3 different kinds of friends - drummers, singers, and
   guitarists.  It takes one of each kind to make a band, plus only
   1 band can be playing at once.  Once those conditions are met, the
   players can start playing and stop playing in any order.  However,
   all 3 players must stop playing before a new set of 3 can start
   playing.

Example output:

drummer arrived
drummer arrived
guitarist arrived
guitarist arrived
singer arrived
drummer playing
guitarist playing
singer playing
singer arrived
singer arrived
drummer arrived
guitarist arrived
drummer finished playing
guitarist finished playing
singer finished playing
singer playing
guitarist playing
drummer playing
singer finished playing
guitarist finished playing
drummer finished playing
guitarist playing
drummer playing
singer playing
guitarist finished playing
drummer finished playing
singer finished playing
Everything finished.


**/

int DRUM = 0;
int SING = 1;
int GUIT = 2;

int drum_count = 0;
int sing_count = 0;
int guit_count = 0;
int done_playing = 0;

// global counts protected by a mutex
sem_t mutex;
sem_t playing_mutex;

// for each band member to wait for a full band
sem_t band;
sem_t drum;
sem_t sing;
sem_t guit;

char* names[] = {"drummer", "singer", "guitarist"};



// because the code is similar, we'll just have one kind of thread
// and we'll pass its kind as a parameter
void* friend(void * kind_ptr) {
    int kind = *((int*) kind_ptr);
    printf("%s arrived\n", names[kind]);
	switch (kind) 
	{
		case 0:	sem_wait(&mutex);
				drum_count++;

				if (drum_count>0 && sing_count>0 && guit_count>0) {
					drum_count--;
					sing_count--;
					guit_count--;
					sem_post(&mutex);
					sem_wait(&band);
					sem_post(&guit);
					sem_post(&sing);
				} else {
					sem_post(&mutex);
					sem_wait(&drum);
				}
				break;
		case 1:	sem_wait(&mutex);
				sing_count++;

				if (drum_count>0 && sing_count>0 && guit_count>0) {
					drum_count--;
					sing_count--;
					guit_count--;
					sem_post(&mutex);
					sem_wait(&band);
					sem_post(&guit);
					sem_post(&drum);
				} else {
					sem_post(&mutex);
					sem_wait(&sing);
				}
				break;
		case 2:	sem_wait(&mutex);
				guit_count++;

				if (drum_count>0 && sing_count>0 && guit_count>0) {
					drum_count--;
					sing_count--;
					guit_count--;
					sem_post(&mutex);
					sem_wait(&band);
					sem_post(&sing);
					sem_post(&drum);
				} else {
					sem_post(&mutex);
					sem_wait(&guit);
				}
				break;
		default: printf("Wrong type\n");
				exit(1);
	}
			
    printf("%s playing\n", names[kind]);
	sleep(1);
    printf("%s finished playing\n", names[kind]);

	sem_wait(&playing_mutex);
	done_playing++;
	if (done_playing == 3) {
		done_playing = 0;
		sem_post(&band);
	}
	sem_post(&playing_mutex);
    return NULL;
}

pthread_t friends[100];
int friend_count = 0;

void create_friend(int* kind) {
    pthread_create(&friends[friend_count], NULL, friend, kind);
    friend_count++;
}

int main(int argc, char **argv) {

	sem_init(&mutex,0,1);
	sem_init(&playing_mutex,0,1);
	sem_init(&band,0,1);
	sem_init(&guit,0,0);
	sem_init(&sing,0,0);
	sem_init(&drum,0,0);
    
    create_friend(&DRUM);
    create_friend(&DRUM);
    create_friend(&GUIT);
    create_friend(&GUIT);
    sleep(1);
    create_friend(&SING);
    create_friend(&SING);
    create_friend(&DRUM);
    create_friend(&GUIT);
    create_friend(&SING);

    // all threads must be created by this point
    // note if you didn't create an equal number of each, we'll be stuck forever
    for (int i = 0; i < friend_count; i++) {
        pthread_join(friends[i], NULL);
    }
	
	sem_destroy(&mutex);
	sem_destroy(&playing_mutex);
	sem_destroy(&band);
	sem_destroy(&guit);
	sem_destroy(&drum);
	sem_destroy(&sing);
	
    printf("Everything finished.\n");

}
