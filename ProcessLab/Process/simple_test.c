#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/wait.h>
#include <stdbool.h>
#include <signal.h>
#include <string.h>

#define simple_assert(message, test) do { if (!(test)) return message; } while (0)
#define TEST_PASSED NULL
#define DATA_SIZE 100
#define INITIAL_VALUE 77
#define MAX_TESTS 10

char* (*test_funcs[MAX_TESTS])(); // array of function pointers that store
                           // all of the tests we want to run
int num_tests = 0;

int data[DATA_SIZE][DATA_SIZE]; // shared data that the tests use

void add_test(char* (*test_func)()) {
    if(num_tests == MAX_TESTS) {
        printf("exceeded max possible tests");
        exit(1);
    }
    test_funcs[num_tests] = test_func;
    num_tests++;
}
// this setup function should run before each test
void setup() {
    printf("starting setup\n");
    for(int i = 0; i < DATA_SIZE; i++) {
        for(int j = 0; j < DATA_SIZE; j++) {
            data[i][j] = INITIAL_VALUE;
        }
    }
    // imagine this function does a lot of other complicated setup
    // that takes a long time
    usleep(3000000);
    
}

char* test1();

void block_alarm() {
    sigset_t mask;
    sigemptyset (&mask);
    sigaddset(&mask, SIGALRM);
    if (sigprocmask(SIG_BLOCK, &mask, NULL) < 0) {
        perror("sigprocmask");
    }
}

void unblock_alarm() {
    sigset_t mask;
    sigemptyset (&mask);
    sigaddset(&mask, SIGALRM);
    if (sigprocmask(SIG_UNBLOCK, &mask, NULL) < 0) {
        perror("sigprocmask");
    }
}

void alrm_handler(int sig_num) {
    exit(3);
}

void run_all_tests() {

    int pipes[num_tests][2]; // pipe for each child process spawned. One pipe is an array (i.e. pipe[2])
    int pids[num_tests];     // store child pid info for use by waitpid later
    signal(SIGALRM, alrm_handler);

    setup();
    for (int i = 0; i < num_tests; i++) {
        pipe(pipes[i]);
        int pid = fork();
        pids[i] = pid;
        if (pid < 0) {
            printf("Fork failed.\n");
            return;
        } 
        // child will split and execute the test functions stored in test_funcs
        else if (pid == 0) {
            int child_pid = fork();
            if (child_pid < 0) {
                printf("Fork failed.\n");
                return;
            } else if (child_pid == 0) { // child will actually run the function at test_funcs[i]
                char* (*test_func)() = test_funcs[i];

                unblock_alarm();
                alarm(3); // set an alarm of 3 sec for test timeout
                char* result = test_func(); // store the result of the test function in a char*
                block_alarm();

                if (result == TEST_PASSED) {
                    close(pipes[i][1]); 
                    exit(0);
                }
                else { 
                    close(pipes[i][0]);
                    write(pipes[i][1],result, strlen(result)); // write the error message to the pipe
                    close(pipes[i][1]);
                    exit(1); 
                }
            } else {
                int status;
                wait(&status);
                if (!WIFEXITED(status)) { exit (2); } // test crashed
                if (WEXITSTATUS(status) == 0) { exit(0); } // test succeeded
                else if (WEXITSTATUS(status) == 3) { exit(3); } // test timed out (via alarm)
                else { exit(1); } // test failed
            }
        } 
        // parent continues and spawns all child processes
        else {
            continue;
        }
    }

    /* parent will loop through and get results back from each child process in order
        child exit code:
        0: Test succeeded and returned normally
        1: Test failed and returned normally; read and print the error message
        2: Test crashed; did not return normally
        3: Test timed out; alarm triggered exit(3)
    */
    int status;
    for (int j = 0; j < num_tests; j++) {
        waitpid(pids[j], &status, 0);
        if (WEXITSTATUS(status) == 0) { printf("Test passed\n"); }
        else if (WEXITSTATUS(status) == 1) { 
            char readBuffer[80];
            close(pipes[j][1]);
            read(pipes[j][0], readBuffer, sizeof(readBuffer));
            printf("Test Failed: %s\n", readBuffer); 
        }
        else if (WEXITSTATUS(status) == 2) { printf("Test Crashed\n"); }
        else if (WEXITSTATUS(status) == 3) { printf("Test timed out\n"); }
    }
        
}

char* test1() {

    printf("starting test 1\n");
    for(int i = 0; i < DATA_SIZE; i++) {
        for(int j = 0; j < DATA_SIZE; j++) {
            simple_assert("test 1 data not initialized properly", data[i][j] == INITIAL_VALUE);
        }
    }

    for(int i = 0; i < DATA_SIZE; i++) {
        for(int j = 0; j < DATA_SIZE; j++) {
            data[i][j] = 1;
        }
    }
    
    for(int i = 0; i < DATA_SIZE; i++) {
        for(int j = 0; j < DATA_SIZE; j++) {
            simple_assert("test 1 data not set properly", data[i][j] == 1);
        }
    }
    printf("ending test 1\n");
    return TEST_PASSED;
}

char* test2() {

    printf("starting test 2\n");
    for(int i = 0; i < DATA_SIZE; i++) {
        for(int j = 0; j < DATA_SIZE; j++) {
            simple_assert("test 2 data not initialized properly", data[i][j] == INITIAL_VALUE);
        }
    }

    for(int i = 0; i < DATA_SIZE; i++) {
        for(int j = 0; j < DATA_SIZE; j++) {
            data[i][j] = 2;
        }
    }

    for(int i = 0; i < DATA_SIZE; i++) {
        for(int j = 0; j < DATA_SIZE; j++) {
            simple_assert("test 2 data not set properly", data[i][j] == 2);
        }
    }

    printf("ending test 2\n");
    return TEST_PASSED;
}

char* test3() {

    printf("starting test 3\n");

    simple_assert("test 3 always fails", 1 == 2);
    
    printf("ending test 3\n");
    return TEST_PASSED;
}


char* test4() {

    printf("starting test 4\n");

    int *val = NULL;
    printf("data at val is %d", *val);
    
    printf("ending test 4\n");
    return TEST_PASSED;
}

char* test5() {

    printf("starting test 5\n");

    while(1) { } 
    
    printf("ending test 5\n");
    return TEST_PASSED;
}

void main() {
    add_test(test1);
    add_test(test2);
    add_test(test3);
    add_test(test4); // uncomment for Step 4
    add_test(test5); // uncomment for Step 5
    run_all_tests();
    
}
