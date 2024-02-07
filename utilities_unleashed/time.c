/**
 * utilities_unleashed
 * CS 341 - Spring 2024
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <time.h>
#include "format.h"

int main(int argc, char *argv[]) {
    // check user gave >= 1 arg
    if (argc < 2) {
        // if not print correct usage of time program and exit
        print_time_usage();
        return 1; // non-zero return is error
    }
    struct timespec start;      // store start time
    struct timespec end;        // store end time
    double duration;            // store total duration in s

    // grab start time before exe
    if (clock_gettime(CLOCK_MONOTONIC, &start) == -1) {
        perror("Failed start clock_gettime");
        return 1; // exit error
    }

    // create child provess
    pid_t pid = fork();
    if (pid == -1) {
        // fork failed
        print_fork_failed();
        return 1;
    } else if (pid == 0) {
        // child process routes here
        // replace child process with new program
        execvp(argv[1], &argv[1]);
        // if returns it failed
        print_exec_failed();
        exit(1); // exit child with error
    } else {
        // exe by parent
        int status;
        // wait for child and get exit status
        waitpid(pid, &status, 0);

        //get end time after command exed
        if (clock_gettime(CLOCK_MONOTONIC, &end) == -1) {
            perror("Failed end clock_gettime");
            return 1;
        }

        // Check status of child process exit, 1 from first WIFEXTITED if terminated correctly and 0 from the WEXITSTATUS if it completed w/o error
        if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
            // calc duration, second then nanosecond calc
            duration = (end.tv_sec - start.tv_sec) + ((end.tv_nsec - start.tv_nsec) / 1E9);
            display_results(argv, duration);
        } else {
            // error return
            return 1;
        }
    }

    return 0;
}
