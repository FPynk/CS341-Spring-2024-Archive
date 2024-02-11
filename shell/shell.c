/**
 * shell
 * CS 341 - Spring 2024
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include "format.h"
#include "shell.h"
#include "vector.h"
#include "sstring.h"

// debug print
void debug_print(const char *msg) {
    #ifdef DEBUG
    printf("DEBUG: %s\n", msg);
    fflush(stdout);
    #endif
}

typedef struct process {
    char *command;
    pid_t pid;
} process;

typedef struct shell_env {
    char *history_file_path;
    size_t initial_history_size;
    char *script_file_path;
    vector *command_history;
} shell_env;

// Helpers
// converts cstr to sstring then push back
// void vector_push_back_cstring(vector *vec, const char* str) {
//     sstring *s_str = cstr_to_sstring(str);
//     vector_push_back(vec, s_str);
//     sstring_destroy(s_str);
// }

// Function prototypes
void parse_arguments(int argc, char * argv[], shell_env *env);
void load_history(shell_env *env);
void save_history(shell_env *env);
void execute_script(shell_env *env);
void catch_sigint(int signum);

// built in commands
int helper_change_directory(const char *path);
int helper_history(const shell_env *env);
int helper_n(const shell_env *env, int n);
int helper_prefix(const shell_env *env, const char *prefix);

// function protos
// Parse and handle -f and -h arguments
void parse_arguments(int argc, char * argv[], shell_env *env) {
    debug_print("Function: parse_arguments");
    int opt;
    while ((opt = getopt(argc, argv, "f:h:")) != -1) {
        switch (opt) {
            case 'f':
                env->script_file_path = strdup(optarg);
                break;
            case 'h':
                env->history_file_path = strdup(optarg);
                break;
            default:
                fprintf(stderr, "Usage: %s [-f script_file] [-h history_file]\n", argv[0]);
                exit(EXIT_FAILURE);
        }
    }
}
// load history from file into env
void load_history(shell_env *env) {
    debug_print("Function: load_history");
    FILE *file = fopen(env->history_file_path, "r");
    if (!file) {
        // treat as empty file
        if (errno == ENOENT) {
            debug_print("Empty cmd hist file");
        } else {
            print_history_file_error();
        }
        return;
    }
    debug_print("Not empty cmd hist file");
    char *line = NULL;
    size_t len = 0;
    ssize_t read;
    while ((read = getline(&line, &len, file)) != -1) {
        // remove newline character from end of line if present
        if (line[read - 1] == '\n') {
            line[read - 1] = '\0';
        }
        // convert line to sstring and add to command history
        sstring *command = cstr_to_sstring(line);
        if (command != NULL) {
            // debug_print("Adding to cmd hist:");
            vector_push_back(env->command_history, command);
        }
        sstring_destroy(command);
    }

    free(line);
    fclose(file);

    // track initial history
    env->initial_history_size = vector_size(env->command_history);
}

void save_history(shell_env *env) {
    debug_print("Function: save_history");
    // opens file to append, will create new file if doesn't exist
    FILE *file = fopen(env->history_file_path, "a");
    if (!file) {
        perror("Cannot open/create history file");
        return;
    }

    // iterate through command history vector and append each command to the file
    for (size_t i = env->initial_history_size; i < vector_size(env->command_history); ++i) {
        sstring *command = (sstring *) vector_get(env->command_history, i);
        if (command == NULL) {
            debug_print("Encountered NULL command in history.");
            continue; // Skip this iteration
        }
        char *command_str = sstring_to_cstr(command);
        debug_print("Adding to cmd hist file:");
        debug_print(command_str);
        fprintf(file, "%s\n", command_str);
        free(command_str);
        sstring_destroy(command);
    }
    fclose(file);
}

void execute_script(shell_env *env) {
    debug_print("Function: execute_script");

    FILE *file = fopen(env->script_file_path, "r");
    if (!file) {
        print_script_file_error();
        return;
    }

    char *line = NULL;
    size_t len = 0;
    ssize_t read;

    while((read = getline(&line, &len, file)) != -1) {
        // remove newline 
        if (line[read - 1] == '\n') {
            line[read - 1] = '\0';
        }
        // figure out command and execute
        if (strncmp(line, "cd", 2) == 0) {
            helper_change_directory(line + 2);
            sstring *s_str = cstr_to_sstring(line);
            vector_push_back(env->command_history, s_str);
            sstring_destroy(s_str);
        } else if (strncmp(line, "!history", 8) == 0) {
            // not stored in hist
            helper_history(env);
        } else  if (line[0] == '#') {
            int n = atoi(line + 1);
            // not stored in hist, store cmd this calls
            helper_n(env, n);
        } else if (line[0] == '!') {
            // not stored in hist, store cmd this calls
            helper_prefix(env, line + 1);
        } else if (strncmp(line, "exit", 4) == 0) {
            break;
        } else {
            // TODO: figure out wtf to do here
            printf("Unrecognized command in script: %s\n", line);
        }
    }
    free(line);
    fclose(file);
}


void catch_sigint(int signum) {

}

// built in commands
// no need to print success
int helper_change_directory(const char *path) {
    debug_print("Function: Helper CD");
    // try to chdir then check result
    if (chdir(path) != 0) {
        // failed check which error
        switch(errno) {
            case ENOENT:
                // directory DNE
                print_no_directory(path);
                break;
            case EACCES:
                debug_print("Perm denied for cd");
                break;
            default:
                debug_print("Can't CD for some reason");
                break;
        }
        return -1; // error return
    }
    // success
    debug_print("Success cd");
    return 0;
}

int helper_history(const shell_env *env) {
    debug_print("Function: Helper History");
    for (size_t i = 0; i < vector_size(env->command_history); ++i) {
        sstring *sstr = (sstring *) vector_get(env->command_history, i);
        char *str = sstring_to_cstr(sstr);
        if (!str) {
            debug_print("Helper history: Why the fuck is this line NULL");
            return -1;
        }
        print_history_line(i, str);
        free(str);
    }
    return 0;
}

int helper_n(const shell_env *env, int n) {
    return 0;
}

int helper_prefix(const shell_env *env, const char *prefix) {
    return 0;
}

    // TODO: This is the entry point for your shell.
    // Print a command prompt
    // Read the command from standard input
    // Print the PID of the process executing the command

    // suport 2 optional arguments (use getopt)
    // -h load history file as history, (STLL KEEP HISTORY if not called, just do not write to file)
    // When exit, append commands of current session into history file
    // -f run series of commands from script file
    // print and run commands in sequential order til EOF
    // if inc no of args or no script file, use format.h print and exit

    // prompting for command: pid & path, no newline ->(pid=<pid>)<path>$
    // reads from stdin or -f for file

    // built-in and external
    // built-in: exe w/o creating new process (No need to print PID)
    // external: exe w/ creating new process (not one of built in listed)
    // If run by new process, print PID: Command executed by pid=<pid>, see format.h
    // Command arguments: whitespace delimited w/o trailing whitespace, no need to support quotes
    // STORE IN HISTORY

    // Exit:
    // Receive exit from stdin
    // EOF at beginning of the line (CTRL-D)
    // or sent from script file
    // If processes still running: KILL and cleanup children before exit
    // ignore sigterm
    // DO NOT STORE EXIT in history

    // Ctrl-c / SIGINT catching
    // Shell itself ignore SIGINT
    // Kill currently running foreground proces
    // Use KILL on foreground when SIGINT Caught
    // Or: do nothing and let natural propagation, to kill only foreground process, use setgpid to assign new process group

    // Built in commands
    // cd <path>
    // if no / then follow relative
    // DNE then print error format.h, treated as nonexistent directory
    // path arg mandatory
    // use system call

    // !history
    // Prints out each command in the history in order
    // DO NOT STORE THIS COMMAND IN HISTORY

    // #<n>
    // prints and exe n-th command of history, chrono order, earliest to most recent
    // n non negative CHECK
    // command exe store again in hist
    // n not valid index prin appropriate error
    // Print out command before exe if match
    // #n itself not stored, just command it exe

    // !<prefix>
    // prints and executes LAST command that has specified prefix
    // no match, print error, do not store in hist
    // prefix may be empty
    // print out cmd b4 exe if match
    // do not store !prefix, store the command you exe

int shell(int argc, char *argv[]) {
    debug_print("Function: shell");
    shell_env env = {0};
    env.command_history = string_vector_create();
    // parse command-line args
    parse_arguments(argc, argv, &env);

    // Load history if specified
    if (env.history_file_path) {
        load_history(&env);
    }

    // exe script file if needed
    if (env.script_file_path) {
        execute_script(&env);
    }

    // TODO: main shell loop

    //save history upon exit
    if (env.history_file_path) {
       for (size_t i = 0; i < vector_size(env.command_history); ++i) {
            // Assuming command_history stores pointers to sstring objects
            sstring* command_sstr = (sstring*) vector_get(env.command_history, i);
            char* command_cstr = sstring_to_cstr(command_sstr); // Convert to C string
            if (command_cstr != NULL) {
                printf("%zu: %s\n", i, command_cstr); // Print the command
                free(command_sstr);
                free(command_cstr); // Free the dynamically allocated C string
            } else {
                printf("%zu: (null)\n", i); // Handle the case where conversion fails
            }
        }
        // save_history(&env);
    }

    // Cleaning up shell
    free(env.history_file_path);
    free(env.script_file_path);
    if (env.command_history) {
        vector_destroy(env.command_history);
    }
    return 0;
}
