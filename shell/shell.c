/**
 * shell
 * CS 341 - Spring 2024
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/wait.h>

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
    int *exit_flag;
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
void erase_last_if_no_match(vector *vec, const char *line);

// built in commands
int command_logical_operators(const shell_env *env, char *line);
int command_line_exe(const shell_env *env, char* line);
int helper_change_directory(const char *path);
int helper_history(const shell_env *env);
int helper_n(const shell_env *env, int n);
int helper_prefix(const shell_env *env, const char *prefix);
void helper_exit(const shell_env *env);

// external commands
int helper_external_command(const shell_env *env, const char *line);

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
                debug_print("default case parse");
                print_usage();
                exit(EXIT_FAILURE);
        }
    }

    if (optind < argc) {
        print_usage();
        exit(EXIT_FAILURE);
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
        vector_push_back(env->command_history, line);
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
        char *command_str = (char *) vector_get(env->command_history, i);
        debug_print("Adding to cmd hist file:");
        debug_print(command_str);
        fprintf(file, "%s\n", command_str);
        // free(command_str);
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
        print_command(line);
        int status = command_logical_operators(env, line);
        if (status == 1 && *(env->exit_flag) == 1) {
            break;
        }
    }
    free(line);
    fclose(file);
}

void catch_sigint(int signum) {
    // does nothing
}

void erase_last_if_no_match(vector *vec, const char *line) {
    // debug_print("Function: erase_last_if_no_match");
    if (vector_size(vec) == 0) {
        // Vector is empty, nothing to do
        return;
    }

    char **last_element = (char **) vector_back(vec);
    // compare the last element with line
    if (strcmp(*last_element, line) != 0) {
        debug_print("Line is:");
        debug_print(line);
        debug_print("Deleting:");
        debug_print(*last_element);
        vector_pop_back(vec);
    }
}

// Figure out history saving
// Figures out logical operator
int command_logical_operators(const shell_env *env, char *line) { // DO NOT EDIT LINE WILL EDIT VECTOR
    char *delimiter;
    int last_exit_status = 0;
    char* whole_command = strdup(line); // remember to free
    // figure out logical operator, assuming only 1 exists
    // && operator
    if ((delimiter = strstr(whole_command, " && ")) != NULL) {
        // split at log operator
        vector_push_back(env->command_history, line);
        *delimiter = '\0'; // splits string at logical operator
        char *first_cmd = whole_command;
        char *second_cmd = delimiter + 4; // && cmd2

        // Exe and saving to history, remove duplicates
        last_exit_status = command_line_exe(env, first_cmd);
        erase_last_if_no_match(env->command_history, line);
        if (last_exit_status == 0) {
            last_exit_status = command_line_exe(env, second_cmd);
            erase_last_if_no_match(env->command_history, line);
        } else if (last_exit_status == 1 && *(env->exit_flag) == 1) {
            // Exit first command
            helper_exit(env);
            return last_exit_status;
        }
    // || operator
    } else if ((delimiter = strstr(whole_command, " || ")) != NULL) {
        vector_push_back(env->command_history, line);
        *delimiter = '\0'; // splits string at logical operator
        char *first_cmd = whole_command;
        char *second_cmd = delimiter + 4; // || cmd2
        
        last_exit_status = command_line_exe(env, first_cmd);
        erase_last_if_no_match(env->command_history, line);
        // if (last_exit_status == 0) {debug_print("LES is 0");}
        // if (last_exit_status == 1) {debug_print("LES is 1");}
        // if (*(env->exit_flag) == 1) {debug_print("env is 1");}
        if (last_exit_status == 1 && *(env->exit_flag) == 1) {
            debug_print("|| Exit");
            // TODO
            // Exit first command
            helper_exit(env);
            return last_exit_status;
        } else if (last_exit_status != 0) {
            debug_print("2nd OR exe");
            last_exit_status = command_line_exe(env, second_cmd);
            erase_last_if_no_match(env->command_history, line);
        }
        debug_print("Should not be here");
    // ; operator
    } else if ((delimiter = strstr(whole_command, "; ")) != NULL) {
        vector_push_back(env->command_history, line);
        *delimiter = '\0'; // splits string at logical operator
        char *first_cmd = whole_command;
        char *second_cmd = delimiter + 2; // ; cmd2
        last_exit_status = command_line_exe(env, first_cmd);
        erase_last_if_no_match(env->command_history, line); // FIX THIS LATER line is not whole line, check the rest
        // TODO EXIT
        if (last_exit_status == 1 && *(env->exit_flag) == 1) {
            helper_exit(env);
            free(whole_command);
            return last_exit_status;
        }

        last_exit_status = command_line_exe(env, second_cmd);
        erase_last_if_no_match(env->command_history, whole_command);
    } else {
        last_exit_status = command_line_exe(env, line);
    }
    // Handling exit
    free(whole_command);
    if (last_exit_status == 1 && *(env->exit_flag) == 1) {
            helper_exit(env);
            return last_exit_status;
    }
    return last_exit_status;
}

// Figures out cmd from line and runs it
// Currently only support some built in commands
int command_line_exe(const shell_env *env, char *line) {
    // figure out command and execute
    int status = 0;
    if (strncmp(line, "cd", 2) == 0) {
        status = helper_change_directory(line + 3);
        vector_push_back(env->command_history, line);
    } else if (strncmp(line, "!history", 8) == 0) {
        // not stored in hist
        status = helper_history(env);
    } else  if (line[0] == '#') {
        int n = atoi(line + 1);
        // not stored in hist, store cmd this calls
        status = helper_n(env, n);
    } else if (line[0] == '!') {
        // not stored in hist, store cmd this calls
        status = helper_prefix(env, line + 1);
    } else if (strncmp(line, "exit", 4) == 0) {
        // not quite sure what to do here
        debug_print("Exit called in file");
        *(env->exit_flag) = 1;
        status = 1;
    } else {
        // Exe external command
        status = helper_external_command(env, line);
        vector_push_back(env->command_history, line);
        // TODO: figure out wtf to do here
        // printf("Unrecognized command in script: %s\n", line);
        // return -1;
    }
    return status;
}

// built in commands
// no need to print success
int helper_change_directory(const char *path) {
    // debug_print("Function: Helper CD");
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
        debug_print("Cd failed");
        return -1; // error return
    }
    // success
    debug_print("Success cd");
    return 0;
}

int helper_history(const shell_env *env) {
    // debug_print("Function: Helper History");
    for (size_t i = 0; i < vector_size(env->command_history); ++i) {
        char *str = (char *) vector_get(env->command_history, i);
        if (!str) {
            debug_print("Helper history: Why the fuck is this line NULL");
            return -1;
        }
        print_history_line(i, str);
    }
    return 0;
}

int helper_n(const shell_env *env, int n) {
    // debug_print("Function: helper_n");
    // printf("%d\n", n);
    if (n < 0 || n >= (int) vector_size(env->command_history)) {
        print_invalid_index();
        return -1;
    }
    char *command = (char *) vector_get(env->command_history, n);
    print_command(command);
    int status = command_logical_operators(env, command);
    return status;
}

int helper_prefix(const shell_env *env, const char *prefix) {
    // debug_print("Function: helper_prefix");
    int found = 0;
    char *command_to_execute = NULL;

    if (prefix[0] == '\0') {
        // If prefix is empty, use the last command in the history, if available
        if (vector_size(env->command_history) > 0) {
            command_to_execute = (char *) vector_get(env->command_history, vector_size(env->command_history) - 1);
            found = 1;
        }
    } else {
        // Iterate in reverse order to find a command starting with the prefix
        for (int i = vector_size(env->command_history) - 1; i >= 0; --i) {
            char *command_str = (char *) vector_get(env->command_history, i);

            // Check if command starts with prefix
            if (strncmp(command_str, prefix, strlen(prefix)) == 0) {
                found = 1;
                command_to_execute = command_str;
                break;
            }
        }
    }

    if (!found) {
        print_no_history_match();
        return -1;
    } else {
        print_command(command_to_execute);
        if (command_logical_operators(env, command_to_execute) != 0) {
            debug_print("Command failed to exe");
            return -1;
        }
        debug_print("Command Success");
        return 0;
    }
}

int helper_external_command(const shell_env *env, const char *line) {
    debug_print("external command");
    pid_t pid;
    int status;
    // prevent double printing due to fork()
    fflush(stdout);
    
    // fork new process
    pid = fork();

    if (pid == -1) {
        // fork failed
        print_fork_failed();
        return -1;
    } else if (pid == 0) {
        // child process
        // reset signal SIGINT
        signal(SIGINT, SIG_DFL);
        // Parse command and arguments
        print_command_executed(getpid());
        char *argv[64]; // max 64 arguments
        int argc = 0;
        char *token = strtok(strdup(line), " "); // strtok modifies string
        while (token != NULL && argc < 63) {
            argv[argc++] = token;
            token = strtok(NULL, " ");
        }
        argv[argc] = NULL;

        // exe cmd
        if (execvp(argv[0], argv) == -1) {
            debug_print("ext exec failed");
            print_exec_failed(argv[0]);
            fflush(stdout);
            exit(EXIT_FAILURE);
        }
    } else {
        // parent process
        // wait for child to finish
        do {
            if (waitpid(pid, &status, 0) == -1) {
                print_wait_failed();
                return -1;
            }
        } while (!WIFEXITED(status) && !WIFSIGNALED(status)); // Check if child hasn't exited normally and child wasnt killed by signal
        if (WIFEXITED(status) && WEXITSTATUS(status) == EXIT_FAILURE) {
            // The child process exited with EXIT_FAILURE, indicating execvp failed
            debug_print("ext failed");
            return -1;
        }
        if (status != 0) {
            debug_print("ext failed 2");
            return -1;
        }
    }
    debug_print("ext success");
    return 0;
}

void helper_exit(const shell_env *env) {
    *(env->exit_flag) = 1;
    if (strstr((char *) vector_back(env->command_history), "exit") != NULL) {
        // "exit" was found
        vector_pop_back(env->command_history);
        debug_print("Exit found, deleting from history");
        return;
    }
    debug_print("Exit NOT found");
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
    signal(SIGINT, catch_sigint);
    shell_env env = {0};
    env.command_history = string_vector_create();
    env.exit_flag = malloc(sizeof(int));
    *(env.exit_flag) = 0;
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
    char cmd_buffer[1024];
    while (*(env.exit_flag) != 1) {
        // todo stuff
        char cwd[1024];
        // print prompt and ask for input
        if (getcwd(cwd, sizeof(cwd)) != NULL) {
            print_prompt(cwd, getpid());
            fflush(stdout);
        } else {
            perror("getcwd() error");
            break;
        }
        // Checkiung for EOF
        if (fgets(cmd_buffer, sizeof(cmd_buffer), stdin) == NULL) {
            if (feof(stdin)) {
                printf("\n");
                *(env.exit_flag) = 1;
            } else {
                perror("fgets error");
            }
            continue;
        }
        cmd_buffer[strcspn(cmd_buffer, "\n")] = 0;
        // newline case
        if (cmd_buffer[0] == '\0') {
            continue;
        }
        if (strcmp(cmd_buffer, "exit") == 0) {
            *(env.exit_flag) = 1;
        } else {
            command_logical_operators(&env, cmd_buffer);
        }
    }
    //save history upon exit
    if (env.history_file_path) {
        save_history(&env);
    }

    // Cleaning up shell
    free(env.history_file_path);
    free(env.script_file_path);
    free(env.exit_flag);
    if (env.command_history) {
        vector_destroy(env.command_history);
    }
    return 0;
}
