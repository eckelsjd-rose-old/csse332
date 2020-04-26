/* Copyright 2016 Rose-Hulman
   But based on idea from http://cnds.eecs.jacobs-university.de/courses/caoslab-2007/
   */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdbool.h>
#include <signal.h>

void handler(int sig_num) {
	int status;
	wait(&status);
}

int main() {
	bool bg;
    char command[82];
    char *parsed_command[2];
	signal(SIGCHLD, handler);
    //takes at most two input arguments
    // infinite loop but ^C quits
    while (1) {
		bg = false;
        printf("SHELL%% ");
        fgets(command, 82, stdin);
        command[strlen(command) - 1] = '\0';//remove the \n
		if (command[0] == 'B' && command[1] == 'G') {
			char* ptr = &command[2];
			parsed_command[0] = ptr;
			bg = true;
		} else {
			bg = false;
			parsed_command[0] = command;
		}

        int len_1;
        for(len_1 = 0;command[len_1] != '\0';len_1++){
            if(command[len_1] == ' ')
                break;
        }

		// fork process and exec the child
		pid_t pid = fork();
		if (pid < 0) {
			printf("Fork failed.\n");
			break;
		} else if (pid == 0) { //child
		
			if (bg) {
				// run in backgroud
				pid_t bg_pid = fork();
				if (bg_pid < 0) {
					printf("Fork failed.\n");
					break;
				} else if (bg_pid == 0) { //child

					if(len_1 == strlen(command)){
						printf("Command is '%s' with no arguments\n", parsed_command[0]); 
						parsed_command[1] = NULL;
						execlp(parsed_command[0],parsed_command[0],NULL);
						perror("Return from execlp() not expected");
						exit(EXIT_FAILURE);
					} else{
						command[len_1] = '\0';
						parsed_command[1] = command + len_1 + 1;
						printf("Command is '%s' with argument '%s'\n", parsed_command[0], parsed_command[1]); 
						execlp(parsed_command[0],parsed_command[0],parsed_command[1],NULL);
						perror("Return from execlp() not expected");
						exit(EXIT_FAILURE);
					}
				} else {
					int status;
					waitpid(bg_pid,&status,0);
					printf("Background process finished.\n");
					exit(0);
				} 
			} else {
				// don't run in background
				if(len_1 == strlen(command)){
					printf("Command is '%s' with no arguments\n", parsed_command[0]); 
					parsed_command[1] = NULL;
					execlp(parsed_command[0],parsed_command[0],NULL);
					perror("Return from execlp() not expected");
					exit(EXIT_FAILURE);
				} else {
					command[len_1] = '\0';
					parsed_command[1] = command + len_1 + 1;
					printf("Command is '%s' with argument '%s'\n", parsed_command[0], parsed_command[1]); 
					execlp(parsed_command[0],parsed_command[0],parsed_command[1],NULL);
					perror("Return from execlp() not expected");
					exit(EXIT_FAILURE);
				}
			}
		} else { //parent
			if (bg) {
				continue;
			} else {
				int status;
				wait(&status);
			}
		}
    }
	// shell failed
	return 1;
}
