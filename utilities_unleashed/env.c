/**
 * utilities_unleashed
 * CS 341 - Spring 2024
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include "format.h"

// helper to replace env vars
// takes string as input and checks if references other env

char* replace_env_var(const char *str) {
    // check if start w %, ref
    if (str[0] != '%') {
        // if not return copy of string
        return strdup(str);
    }

    // ref case:
    // grab val of env with str after %
    const char* env_val = getenv(str + 1);
    // ref env var is NULL/ does not exist
    if (!env_val) {
        return strdup("");
    }
    // return dup of env_val string
    return strdup(env_val);
}

int main(int argc, char *argv[]) {

    // check min args provided
    // min cmd line: ./env key=val -- cmd
    if (argc < 4) {
        print_env_usage();
        return 1;
    }
    // idx in argv where command to be executed starts
    int cmd_idx = 0;

    // iterate over all args to find and set env till --
    for (int i = 1; i < argc; i++) {
        // check if reached breakpoint --
        if (strcmp(argv[i], "--") == 0) {
            cmd_idx = i + 1; // set to idx aft --
            break;
        }

        // split arg into key and val using = delim
        char* key_val_pair = strdup(argv[i]);
        char* key = strtok(key_val_pair, "=");
        char* val = strtok(NULL, "=");

        // invalid format checking
        if (!key || !val) {
            print_env_usage();
            free(key_val_pair);
            return 1;
        }

        // provess value for env var ref
        char* proc_val = replace_env_var(val);
        // set env var and print err msg if relevant
        if (setenv(key, proc_val, 1)) {
            print_environment_change_failed();
            free(proc_val);
            free(key_val_pair);
            return 1;
        }
        // free alloced space
        free(proc_val);
        free(key_val_pair);
    }

    // check command was provided after --
    if (cmd_idx == 0 || cmd_idx == argc) {
        print_env_usage();
        return 1;
    }

    // fork to exe cmd
    pid_t pid = fork();
    if (pid == -1) {
        print_fork_failed();
        return 1;
    } else if (pid == 0) {
        // child process
        execvp(argv[cmd_idx], &argv[cmd_idx]);
        print_exec_failed(); // shouldn't return
        exit(1);
    } else {
        // parent process
        int status;
        waitpid(pid, &status, 0);

        // check child process eixt
        if (WIFEXITED(status) && WEXITSTATUS(status) != 0) {
            return WEXITSTATUS(status); // return child exit status
        }
    }
    // successful
    return 0;
}
