/**
 * mapreduce
 * CS 341 - Spring 2024
 */
#include "utils.h"
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>

int main(int argc, char **argv) {
    // Check correct arugments provided
    if (argc != 6) {
        print_usage();
    }
    // Assigns input arguments to variables
    char *infile = argv[1];
    char *outfile = argv[2];
    char *map_exe = argv[3];
    char *red_exe = argv[4];
    int map_count = atoi(argv[5]);

    // Create an input pipe for each mapper.
    // array for the pipes
    int *fds[map_count];
    for (int i = 0; i < map_count; ++i) {
        fds[i] = malloc(2 * sizeof(int)); // pipe size is 2 ints
        pipe(fds[i]); // create pipe
    }

    // Create one input pipe for the reducer.
    int r_fd[2];
    pipe(r_fd);

    // Open the output file.
    int out_fd = open(outfile, O_CREAT | O_TRUNC | O_WRONLY, S_IWUSR | S_IRUSR);

    // Start a splitter process for each mapper.
    pid_t split_procs[map_count];
    for (int i = 0; i < map_count; ++i) {
        split_procs[i] = fork();
        // child process
        if (split_procs[i] == 0) {
            close(fds[i][0]); // read not needed
            char buffer[64];
            sprintf(buffer, "%d", i);
            // replace stdout with pipe write end and exe splitter
            dup2(fds[i][1], STDOUT_FILENO);
            execl("./splitter", "./splitter", infile, argv[5], buffer, NULL);
            exit(0);
        }
    }

    // Start all the mapper processes.
    pid_t map_procs[map_count]; // hold pids of mapper processes
    for (int i = 0; i < map_count; ++i) {
        map_procs[i] = fork();
        close(fds[i][1]); // close write end of pipe, not needed for mapper
        // child process stuff
        if (map_procs[i] == 0) {
            // does this work?
            close(fds[i][1]); // close write end of pipe, not needed for mapper
            close(r_fd[0]); // close read end of reducer pipe
            // set up mapper's stdin and stdout to appropriate pipes
            dup2(fds[i][0], STDIN_FILENO);
            dup2(r_fd[1], STDOUT_FILENO);
            // run mapper and exit
            execl(map_exe, map_exe, NULL);
            exit(0);
        }
    }

    // Start the reducer process.
    close(r_fd[1]); // close write end of reducer pipe
    pid_t r_proc = fork();
    // child process
    if (r_proc == 0) {
        // set up reducers stdin from pipe and stdout to outfile
        dup2(r_fd[0], STDIN_FILENO);
        dup2(out_fd, STDOUT_FILENO);
        execl(red_exe, red_exe, NULL);
        exit(0);
    }
    // close read end of reducer pipe and out file descriptor
    close(r_fd[0]);
    close(out_fd);

    // wait for splitter processes to finish
    // close fds pipe both ends
    for (int i = 0; i < map_count; ++i) {
        int tmp;
        close(fds[i][0]);
        waitpid(map_procs[i], &tmp, 0);
    }

    // Wait for the reducer to finish.
    int stat;
    waitpid(r_proc, &stat, 0);
    // Print nonzero subprocess exit codes.
    if (stat) print_nonzero_exit_status(red_exe, stat);
    // Count the number of lines in the output file.
    print_num_lines(outfile);
    for (int i = 0; i < map_count; ++i) {
        free(fds[i]);
    }
    return 0;
}