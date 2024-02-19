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
#include <ctype.h>
#include <dirent.h>
#include <fcntl.h>

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
    int nlwp;
    unsigned long vsz;
    char stat;
    char *startTime;
    char *cpuTime;
} process;

typedef struct shell_env {
    char *history_file_path;
    size_t initial_history_size;
    char *script_file_path;
    vector *command_history;
    int *exit_flag;
    vector *background_PIDs;
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
void reap_zombie_processes();
void reap_background_processes(shell_env *env);
int is_pid_folder(const char *name);
unsigned long long getSystemBootTime();
char * convert_start_time(unsigned long long int start_time);
char * convert_cpu_time(unsigned long int utime, unsigned long int stime);

// built in commands
int command_logical_operators(const shell_env *env, char *line);
int command_line_exe(const shell_env *env, char* line);
int helper_change_directory(const char *path);
int helper_history(const shell_env *env);
int helper_n(const shell_env *env, int n);
int helper_prefix(const shell_env *env, const char *prefix);
void helper_exit(const shell_env *env);
int helper_ps(const shell_env *env);

// external commands
int is_background_command(const char *cmd);
int helper_external_command(const shell_env *env, const char *line);

// redirection
int output_redirection(const shell_env *env, const char *line);
int append_redirection(const shell_env *env, const char *line);
int input_redirection(const shell_env *env, const char *line);

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

void reap_zombie_processes() {
    int status;
    while(waitpid(-1, &status, WNOHANG) > 0) {
        // reap zombie processes w/o blocking
    }
}

void reap_background_processes(shell_env *env) {
    pid_t pid;
    int status;
    // cycle thru all and clean up
    for (int i = 0; i < vector_size(env->background_PIDs); ++i) {
        pid_t* pid_ptr = vector_get(env->background_PIDs, i); // may need to free
        pid_t pid = *pid_ptr;
        if (waitpid(pid, &status, WNOHANG) > 0) {
            // Process has finished, perform any additional handling
            vector_erase(env->background_PIDs, i);
        } else {
            // Process has not finished, move to next
            ++i;
        }
    }

}

// Checks if folder is a PID
int is_pid_folder(const char *name) {
    // check all chars are numbers
    for (int i = 0; name[i]; ++i) {
        if (name[i] < '0' || name[i] > '9') return 0;
    }
    return 1;
}

unsigned long long getSystemBootTime() {
    FILE *f_stat = fopen("/proc/stat", "r");
    if (!f_stat) return 0;

    char buffer[256];
    unsigned long long btime = 0;
    // read line by line, find btime and grab data using scanf then break
    while (fgets(buffer, sizeof(buffer), f_stat)) {
        if (strncmp(buffer, "btime", 5) == 0) {
            sscanf(buffer, "btime %llu", &btime);
            break;
        }
    }
    fclose(f_stat);
    return btime;
}
// converts start time (btime + starttime) from jiffies to HH:MM format
char * convert_start_time(unsigned long long int start_time) {
    // system clock ticks per sec
    long hz = sysconf(_SC_CLK_TCK);
    // get boot time in sec since epoch
    unsigned long long btime = getSystemBootTime();
    // calc proc start time in seconds since epoch
    unsigned long long start_seconds = btime + (start_time / hz);

    // calc local time for start_seocnds
    struct tm *start_tm = localtime((time_t *) &start_seconds);

    // allocate buffer for HH:MM format: 5 chars + 1 null
    char *buffer = malloc(6 * sizeof(char));
    if (!buffer) return NULL;
    // format the time into HH:MM
    // snprintf(buffer, 6, "%02d:%02d", start_tm->tm_hour, start_tm->tm_min);
    time_struct_to_string(buffer, 6, start_tm);
    return buffer;
}

char * convert_cpu_time(unsigned long int utime, unsigned long int stime) {
    // clock ticks
    long hz = sysconf(_SC_CLK_TCK);
    // convert user time and sys time form clock ticks to secs
    unsigned long total_time_seconds = (utime + stime)/ hz;
    // calc mins and secs
    unsigned long minutes = total_time_seconds / 60;
    unsigned long seconds = total_time_seconds % 60;
    // Buffer for M:SS 
    char *buffer = malloc(5 * sizeof(char));
    if (!buffer) return NULL;

    // format time
    // snprintf(buffer, 5, "%lu:%02lu", minutes, seconds);
    execution_time_to_string(buffer, 5, minutes, seconds);
    return buffer;
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
    // NOTE: Only external commands for the following redirection
    // > OUTPUT
    } else if ((delimiter = strstr(whole_command, " > ")) != NULL) {
        vector_push_back(env->command_history, line);
        last_exit_status = output_redirection(env, line);
    // >> APPEND
    } else if ((delimiter = strstr(whole_command, " >> ")) != NULL) {
        vector_push_back(env->command_history, line);
        last_exit_status = append_redirection(env, line);
    // < INPUT
    } else if ((delimiter = strstr(whole_command, " < ")) != NULL) {
        vector_push_back(env->command_history, line);
        last_exit_status = input_redirection(env, line);
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
    } else if (strncmp(line, "ps", 2) == 0) {
        status = helper_ps(env);
        vector_push_back(env->command_history, line);
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

int helper_ps(const shell_env *env) {
    // Need these things
    // PID: The pid of the process
    // NLWP: The number of threads currently being used in the process
    // VSZ: The program size (virtual memory size) of the process, in kilobytes (1 kilobyte = 1024 bytes)
    // STAT: The state of the process
    // START: The start time of the process. You will want to add the boot time of the computer (btime), and start time of the process (starttime) to calculate this. Make sure you are careful while converting from various formats - the man pages for procfs have helpful tips.
    // TIME: The amount of cpu time that the process has been executed for. This includes time the process has been scheduled in user mode (utime) and kernel mode (stime).
    // COMMAND: The command that executed the process

    // open /proc directory, contians sub dirs for each running process, named by PID
    DIR *d;
    struct dirent *dir;
    d = opendir("/proc");
    if (!d) {
        // failed to open 
        debug_print("Failed to open /proc directory");
        return -1;
    }

    // print header for process info table
    print_process_info_header();
    // iterate thru dirs till end
    while ((dir = readdir(d)) != NULL) {
        // we only care about dirs and check if its a PID
        if (dir->d_type == DT_DIR && is_pid_folder(dir->d_name)) {
            // store path of stat file
            char path[256];
            // write path with format to path
            snprintf(path, sizeof(path), "/proc/%s/stat", dir->d_name);
            // open file and check
            FILE *f = fopen(path, "r");
            if (!f) continue; // if fail, skip and continue to next one

            process_info p;
            unsigned long int utime;
            unsigned long int stime;
            unsigned long long int start_time;
            // parse stat file
            fscanf(f,
                   "%d %*s %c %*d %*d %*d %*d %*d %*u %*lu %*lu %*lu %*lu %lu %lu %*ld %*ld %*ld %*ld %ld %*ld %llu %lu %*ld",
                   &p.pid, &p.state, &utime, &stime, &p.nthreads, &start_time, &p.vsize);
            fclose(f);
            // conversions to proper format
            p.start_str = convert_start_time(start_time);
            p.time_str = convert_cpu_time(utime, stime);

            // parse comm file
            char comm[256];
            snprintf(path, sizeof(path), "/proc/%s/comm", dir->d_name);
            FILE *f_comm = fopen(path, "r");
            if (f_comm) {
                if (fgets(comm, sizeof(comm), f_comm)) {
                    // remove newline
                    comm[strcspn(comm, "\n")] = 0;
                    p.command = strdup(comm);
                    fclose(f_comm);
                    if (p.command == NULL) continue; // fail mem alloc
                }
            } else {
                debug_print("Failed to open comm file");
                p.command = strdup("Unknown"); // Fallback command name
                if (p.command == NULL) continue; // fail mem alloc
            }

            // print
            print_process_info(&p);

            // cleanup
            free(p.start_str);
            free(p.time_str);
            free(p.command);
        }
    }
    closedir(d);
    return 0;
}

int helper_external_command(const shell_env *env, const char *line) {
    debug_print("external command");
    pid_t pid;
    int status;
    char *command = strdup(line); // remember to free
    // Handling background processes 
    int background = is_background_command(line);
    // removing & from last part
    if (background) {
        debug_print("background command detected");
        char *ampersand = strrchr(command, '&');
        if (ampersand) *ampersand = '\0';
    }
    // prevent double printing due to fork()
    fflush(stdout);
    
    // fork new process
    pid = fork();

    if (pid == -1) {
        // fork failed
        print_fork_failed();
        free(command);
        return -1;
    } else if (pid == 0) {
        // child process
        // reset signal SIGINT
        signal(SIGINT, SIG_DFL);
        // Parse command and arguments
        print_command_executed(getpid());
        char *argv[64]; // max 64 arguments
        int argc = 0;
        char *token = strtok(strdup(command), " "); // strtok modifies string
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
        // foreground
        if (!background) {
            do {
                if (waitpid(pid, &status, 0) == -1) {
                    print_wait_failed();
                    free(command);
                    return -1;
                }
            } while (!WIFEXITED(status) && !WIFSIGNALED(status)); // Check if child hasn't exited normally and child wasnt killed by signal
            if (WIFEXITED(status) && WEXITSTATUS(status) == EXIT_FAILURE) {
                // The child process exited with EXIT_FAILURE, indicating execvp failed
                debug_print("ext failed");
                free(command);
                return -1;
            }
            if (status != 0) {
                debug_print("ext failed 2");
                free(command);
                return -1;
            }
        // Background
        } else {
            vector_push_back(env->background_PIDs, &pid);
            debug_print("Background ext process");
        }
    }
    debug_print("ext success");
    free(command);
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

int is_background_command(const char *cmd) {
    debug_print("Function: is_background_command");
    const char *end = cmd + strlen(cmd) - 1; // grab last char
    // skip trailing whitespace
    while (end > cmd && isspace((unsigned char) *end)) {
        --end;
    }
    // check last non space char is &
    return *end == '&';
}

int output_redirection(const shell_env *env, const char *line) {
    debug_print("output external command");
    // split the line ito command and filename
    pid_t pid;
    int status;
    char *command = strdup(line);
    char *delimiter = strstr(command, " > ");
    if (delimiter == NULL) {
        free(command);
        return -1;      // we should not reach this
    }
    *delimiter = '\0';
    char *filename = delimiter + 3; // skip " > "
    // Handling background processes 
    int background = is_background_command(command);
    // removing & from last part
    if (background) {
        debug_print("output background command detected");
        char *ampersand = strrchr(command, '&');
        if (ampersand) *ampersand = '\0';
    }
    // prevent double printing due to fork()
    fflush(stdout);

    // external command customised for output
    pid = fork();
    if (pid == -1) {
        // fork fails
        print_fork_failed();
        free(command);
        return -1;
    } else if (pid == 0) {
        signal(SIGINT, SIG_DFL);
        print_command_executed(getpid());
        // open file and handle errors
        int fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644); // Check last arg if it breaks test cases
        if (fd == -1) {
            print_redirection_file_error();
            exit(EXIT_FAILURE);
        }

        // redirect stdout to file
        // dup2 closes stdout, duplicates fd to it
        if (dup2(fd, STDOUT_FILENO) == -1) {
            debug_print("Error output dup2");
            exit(EXIT_FAILURE);
        }
        // no longer needed
        close(fd);

        // execute command
        char *argv[64]; // max 64 arguments
        int argc = 0;
        char *token = strtok(strdup(command), " "); // strtok modifies string
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
        // foreground
        if (!background) {
            do {
                if (waitpid(pid, &status, 0) == -1) {
                    print_wait_failed();
                    free(command);
                    return -1;
                }
            } while (!WIFEXITED(status) && !WIFSIGNALED(status)); // Check if child hasn't exited normally and child wasnt killed by signal
            if (WIFEXITED(status) && WEXITSTATUS(status) == EXIT_FAILURE) {
                // The child process exited with EXIT_FAILURE, indicating execvp failed
                debug_print("ext failed");
                free(command);
                return -1;
            }
            if (status != 0) {
                debug_print("ext failed 2");
                free(command);
                return -1;
            }
        // Background
        } else {
            debug_print("Background ext process");
        }
    }
    debug_print("output ext success");
    free(command);
    return 0;
}
int append_redirection(const shell_env *env, const char *line) {
    debug_print("append external command");
    // split the line ito command and filename
    pid_t pid;
    int status;
    char *command = strdup(line);
    char *delimiter = strstr(command, " >> ");
    if (delimiter == NULL) {
        free(command);
        return -1;      // we should not reach this
    }
    *delimiter = '\0';
    char *filename = delimiter + 4; // skip " >> "
    // Handling background processes 
    int background = is_background_command(command);
    // removing & from last part
    if (background) {
        debug_print("append background command detected");
        char *ampersand = strrchr(command, '&');
        if (ampersand) *ampersand = '\0';
    }
    // prevent double printing due to fork()
    fflush(stdout);

    // external command customised for output
    pid = fork();
    if (pid == -1) {
        // fork fails
        print_fork_failed();
        free(command);
        return -1;
    } else if (pid == 0) {
        signal(SIGINT, SIG_DFL);
        print_command_executed(getpid());
        // open file and handle errors
        int fd = open(filename, O_WRONLY | O_CREAT | O_APPEND, 0644); // Check last arg if it breaks test cases
        if (fd == -1) {
            print_redirection_file_error();
            exit(EXIT_FAILURE);
        }

        // redirect stdout to file
        // dup2 closes stdout, duplicates fd to it
        if (dup2(fd, STDOUT_FILENO) == -1) {
            debug_print("Error output dup2");
            exit(EXIT_FAILURE);
        }
        // no longer needed
        close(fd);

        // execute command
        char *argv[64]; // max 64 arguments
        int argc = 0;
        char *token = strtok(strdup(command), " "); // strtok modifies string
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
        // foreground
        if (!background) {
            do {
                if (waitpid(pid, &status, 0) == -1) {
                    print_wait_failed();
                    free(command);
                    return -1;
                }
            } while (!WIFEXITED(status) && !WIFSIGNALED(status)); // Check if child hasn't exited normally and child wasnt killed by signal
            if (WIFEXITED(status) && WEXITSTATUS(status) == EXIT_FAILURE) {
                // The child process exited with EXIT_FAILURE, indicating execvp failed
                debug_print("ext failed");
                free(command);
                return -1;
            }
            if (status != 0) {
                debug_print("ext failed 2");
                free(command);
                return -1;
            }
        // Background
        } else {
            debug_print("Background ext process");
        }
    }
    debug_print("append ext success");
    free(command);
    return 0;
}
int input_redirection(const shell_env *env, const char *line) {
        debug_print("input external command");
    // split the line ito command and filename
    pid_t pid;
    int status;
    char *command = strdup(line);
    char *delimiter = strstr(command, " < ");
    if (delimiter == NULL) {
        free(command);
        return -1;      // we should not reach this
    }
    *delimiter = '\0';
    char *filename = delimiter + 3; // skip " < "
    // Handling background processes 
    int background = is_background_command(command);
    // removing & from last part
    if (background) {
        debug_print("input background command detected");
        char *ampersand = strrchr(command, '&');
        if (ampersand) *ampersand = '\0';
    }
    // open input file and handle errors
    int fd = open(filename, O_RDONLY); // Check last arg if it breaks test cases
    if (fd == -1) {
        print_redirection_file_error();
        free(command);
        return -1;
    }

    // prevent double printing due to fork()
    fflush(stdout);

    // external command customised for output
    pid = fork();
    if (pid == -1) {
        // fork fails
        print_fork_failed();
        free(command);
        return -1;
    } else if (pid == 0) {
        // child
        signal(SIGINT, SIG_DFL);
        print_command_executed(getpid());
        
        // redirect stdout to file
        // dup2 closes stdin, duplicates fd to it
        if (dup2(fd, STDIN_FILENO) == -1) {
            debug_print("Error input dup2");
            exit(EXIT_FAILURE);
        }
        // no longer needed
        close(fd);

        // execute command
        char *argv[64]; // max 64 arguments
        int argc = 0;
        char *token = strtok(strdup(command), " "); // strtok modifies string
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
        close(fd);
        // foreground
        if (!background) {
            do {
                if (waitpid(pid, &status, 0) == -1) {
                    print_wait_failed();
                    free(command);
                    return -1;
                }
            } while (!WIFEXITED(status) && !WIFSIGNALED(status)); // Check if child hasn't exited normally and child wasnt killed by signal
            if (WIFEXITED(status) && WEXITSTATUS(status) == EXIT_FAILURE) {
                // The child process exited with EXIT_FAILURE, indicating execvp failed
                debug_print("ext failed");
                free(command);
                return -1;
            }
            if (status != 0) {
                debug_print("ext failed 2");
                free(command);
                return -1;
            }
        // Background
        } else {
            debug_print("input ext process");
        }
    }
    debug_print("input ext success");
    free(command);
    return 0;
}

// Handle EOF/  ctrl D / exit for background
// exit
// The shell will exit once it receives the exit command or once it receives an EOF 
// at the beginning of the line. An EOF is sent by typing Ctrl-D from your terminal. 
// It is also sent automatically from a script file (as used with the -f flag) once 
// the end of the file is reached. This should cause your shell to exit with exit status 0.
// If there are currently stopped or running background processes when your shell receives 
// exit or Control-D (EOF), you should kill and cleanup each of those children before 
// your shell exits. You do not need to worry about SIGTERM.
// :warning: If you don’t handle EOF or exit to exit, you will fail many of our test cases!
// :warning: Do not store exit in history!

// Catching Ctrl+C
// Usually when we do Ctrl+C, the current running program will exit. However, we want the shell 
// itself to ignore the Ctrl+C signal (SIGINT) - instead, it should kill the currently running 
// foreground process (if one exists) using SIGINT. One way to do this is to use the kill 
// function on the foreground process PID when SIGINT is caught in your shell. However, 
// when a signal is sent to a process, it is sent to all processes in its process group. 
// In this assignment, the shell process is the leader of a process group consisting of 
// all processes that are fork‘d from it. So another way to properly handle Ctrl+C is to 
// simply do nothing inside the handler for SIGINT if it is caught in the shell - your shell 
// will continue running, but SIGINT will automatically propagate to the foreground process 
// and kill it.
// However, since we want this signal to be sent to only the foreground process, but not to any 
// backgrounded processes, you will want to use setpgid to assign each background process to its 
// own process group after forking. (Note: think about who should be making the setpgid call and why).

int shell(int argc, char *argv[]) {
    debug_print("Function: shell");
    signal(SIGINT, catch_sigint);
    shell_env env = {0};
    env.command_history = string_vector_create();
    env.background_PIDs = int_vector_create();
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
        reap_zombie_processes(&env);
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
        reap_zombie_processes(&env);
    }
    reap_background_processes(&env);
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
    if (env.background_PIDs) {
        vector_destroy(env.background_PIDs);
    }
    return 0;
}
