/* Notes
1) Use anonymous POSIX pipes
    - compile: clang -o badpiper -O0 -Wall -Wextra -Werror -g -std=c99 -D_GNU_SOURCE -DDEBUG badpiper.c 
2) Args: Filename, Command1, Command2
    - check file exists
    - command eg: head,tail,wc,uniq,sort,shasum
    - Eg use ./badpiper input.txt sort head
3) file content -> stdin of Command1
    - if cannot open, use 999 bytes from /dev/urandom if DNE, /etc/services for all other access errors
4) 999 bytes of stdout from Command1 -> stdin Command2
    - store 999 bytes output from Command1 in NEW file mypassword in Home directory
    - use HOME env variable
    - overwrite if already exists
5) ALL bytes of Command2 -> stdout
    - redact all decimal digits 0-9 and replace with *
6) See PDF for grading rubric
7) Add netid as comment
8) possible useful functions:  pipe dup2 fork execlp waitpid open read write.
*/
/* GRADING RUBRIC CHECKLIST
A) 10 The included author name comment in your source file is your netid. DONE
B) 10  If 3 arguments are not specified, display a usage message on stderr and exit with a value of 1. DONE
C) 10  Sends all of the input file contents as stdin stream to the first command, except… DONE
D) 10 Sends 999B from /dev/urandom if the input file does not exist, or… DONE
E) 10 Sends 999B from /etc/services if the input file exists but cannot be opened for reading. DONE
F) 10 Uses up to the first 999B of output from the first command as the stdin stream for the second command. DONE
G) 10 Also stores these output bytes in a new file, $HOME/mypassword DONE
H) 10 Displays the entire output of the second command (without any limit) but with digits replaced with an 
asterisk DONE
I) 10 Closes anonymous POSIX pipes for input whenever there will be no more input for that pipe. DONE
J) 10 If either or both commands cannot be executed then exit with value 2, otherwise exit with value 0.DONE
*/ 
// DO NOT REMOVE BELOW
// RUBRIC A)
// author: acloh2

// imports
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>

// constants and definitions
#define BLOCK_SIZE 1024 * 4 // 4KB

// apolgy to the grader that needs to look through this THICC main function
int main(int argc, char *argv[]) {
    //  check number of args correct
    // RUBRIC B)
    if (argc != 4) {
        fprintf(stderr, "Usage: %s <filename> <command1> <command2>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    // initialise variables
    char *filename = argv[1];
    char *cmd_one = argv[2];
    char *cmd_two = argv[3];

    // check file exists
    // RUBRIC C)
    int send_only_999 = 0;
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        // RUBRIC D)
        // RUBRIC E)
        if (errno == ENOENT) {
            perror("Attempting to open urandom\n");
            file = fopen("/dev/urandom", "r");
            if (!file) {
                perror("Failed to open /dev/urandom");
                exit(EXIT_FAILURE);
            }
        } else {
            perror("Attempting to open services\n");
            file = fopen("/etc/services", "r");
            if (!file) {
                perror("Failed to open /etc/services");
                exit(EXIT_FAILURE);
            }
        }
        send_only_999 = 1;
    }
    // fprintf(stderr, "send_only_999: %d\n", send_only_999);
    // setup for mypassword
    char *home_dir = getenv("HOME");
    if (home_dir == NULL) {
        perror("getenv error\n");
        exit(EXIT_FAILURE);
    }

    // construct full path and create file
    char file_path[1024];
    snprintf(file_path, sizeof(file_path), "%s/mypassword", home_dir);

    // setup pipes
    int pipe_one[2];
    int pipe_two[2];

    if (pipe(pipe_one) == -1 || pipe(pipe_two) == -1) {
        perror("pipe error\n");
        return EXIT_FAILURE;
    }
    // read/ write to pipe to command1, check flag
    pid_t pid = fork();
    if (pid == 0) {
        // Child
        // pipe one handling
        close(pipe_one[1]); // close write
        dup2(pipe_one[0], STDIN_FILENO); // redirect to stdin 
        close(pipe_one[0]); // close original read

        // pipe two handling
        close(pipe_two[0]); // close read
        dup2(pipe_two[1], STDOUT_FILENO); // redirect stdout to pipe two
        close(pipe_two[1]);

        // exe command
        execlp(cmd_one, cmd_one, NULL);
        perror("Cmd 1 execlp error\n");
        exit(2);
    } else {
        // Parent
        close(pipe_one[0]); // close read
        close(pipe_two[1]); // close write
        char buffer[BLOCK_SIZE];
        size_t bytes_read = 0;
        size_t total_bytes_read = 0;
        if (send_only_999) {
            // send only 999 bytes
            while((bytes_read = fread(buffer, 1, 999 - total_bytes_read, file)) > 0 && total_bytes_read < 999) {
                ssize_t bytes_written = write(pipe_one[1], buffer, bytes_read);
                if (bytes_written == -1) {
                    perror("Failed to write 999 to mypwd pipe\n");
                    close(pipe_one[1]);
                    fclose(file);
                    exit(EXIT_FAILURE);
                }
                total_bytes_read += bytes_read;
            }
        } else {
            // send unknown bytes
            while ((bytes_read = fread(buffer, 1, 999 - total_bytes_read, file)) > 0) {
                ssize_t bytes_written = write(pipe_one[1], buffer, bytes_read);
                if (bytes_written == -1) {
                    perror("Failed to write to mypwd pipe\n");
                    close(pipe_one[1]);
                    fclose(file);
                    exit(EXIT_FAILURE);
                }
            }
        }
        close(pipe_one[1]);
        fclose(file);
        
        // check child process exit
        // RUBRIC J)
        int status = 0;
        pid_t result = waitpid(pid, &status, 0);
        if (result == -1) {
            perror("failed to wait for cmd1\n");
            exit(2);
        }
        if (WIFEXITED(status)) {
            int exit_status = WEXITSTATUS(status);
            if (exit_status == 2) {
                fprintf(stderr, "Child exited with status %d\n", exit_status);
                exit(2);
            }
        }

        // open file for writing, overwrite if exists
        FILE *pwd_file = fopen(file_path, "w");
        if (pwd_file == NULL) {
            perror("pwd_file error\n");
            close(pipe_two[0]);
            exit(EXIT_FAILURE);
        }

        // Read write from pipe to file
        // RUBRIC G)
        memset(buffer, 0, BLOCK_SIZE);
        bytes_read = 0;
        total_bytes_read = 0;
        while((bytes_read = read(pipe_two[0], buffer, 999 - total_bytes_read)) > 0 && total_bytes_read < 999) {
            ssize_t bytes_written = fwrite(buffer, 1, bytes_read, pwd_file);
            if (bytes_written == -1) {
                perror("Failed to write to mypwd file\n");
                close(pipe_two[0]);
                fclose(pwd_file);
                exit(EXIT_FAILURE);
            }
            total_bytes_read += bytes_read;
        }
        // close pipe read and file
        close(pipe_two[0]);
        fclose(pwd_file);
    }

    // open file for reading
    file = fopen(file_path, "r");
    if (file == NULL) {
        perror("error opening pwd file read\n");
        exit(EXIT_FAILURE);
    }

    // setup pipes
    if (pipe(pipe_one) == -1 || pipe(pipe_two) == -1) {
        perror("pipe error\n");
        exit(EXIT_FAILURE);
    }
    pid = fork();
    if (pid == -1) {
        perror("2nd fork failed\n");
        exit(EXIT_FAILURE);
    }
    if (pid == 0) {
        // Child
        // pipe one handling
        close(pipe_one[1]); // close write
        dup2(pipe_one[0], STDIN_FILENO); // redirect to stdin 
        close(pipe_one[0]); // close original read

        // pipe two handling
        close(pipe_two[0]); // close read
        dup2(pipe_two[1], STDOUT_FILENO); // redirect stdout to pipe two
        close(pipe_two[1]);

        // exe command
        execlp(cmd_two, cmd_two, NULL);
        perror("Cmd 2 execlp error\n");
        exit(2);
    } else {
        // Parent
        close(pipe_one[0]); // close read
        close(pipe_two[1]); // close write
        char buffer[BLOCK_SIZE];
        size_t bytes_read = 0;
        size_t total_bytes_read = 0;
        // send only 999 bytes to cmd2
        while((bytes_read = fread(buffer, 1, 999 - total_bytes_read, file)) > 0 && total_bytes_read < 999) {
            ssize_t bytes_written = write(pipe_one[1], buffer, bytes_read);
            if (bytes_written == -1) {
                perror("Failed to write to cmd2 pipe\n");
                close(pipe_one[1]);
                fclose(file);
                exit(EXIT_FAILURE);
            }
            // ignore write fail, not def by spec
            total_bytes_read += bytes_read;
        }

        close(pipe_one[1]);
        fclose(file);

        // check child process exit
        // RUBRIC J)
        int status = 0;
        pid_t result = waitpid(pid, &status, 0);
        if (result == -1) {
            perror("failed to wait for cmd2\n");
            exit(2);
        }
        if (WIFEXITED(status)) {
            int exit_status = WEXITSTATUS(status);
            if (exit_status == 2) {
                fprintf(stderr, "Child exited with status %d\n", exit_status);
                exit(2);
            }
        }

        // Read write from pipe to stdout
        // RUBRIC G)
        memset(buffer, 0, BLOCK_SIZE);
        bytes_read = 0;
        while((bytes_read = read(pipe_two[0], buffer, BLOCK_SIZE)) > 0) {
            // replace digits with *
            for (size_t i = 0; i < bytes_read; ++i) {
                if (buffer[i] >= '0' && buffer[i] <= '9') {
                    buffer[i] = '*';
                }
            }
            // write to stdout
            ssize_t bytes_written = write(STDOUT_FILENO, buffer, bytes_read);
            if (bytes_written == -1) {
                perror("write to stdout failed\n");
                close(pipe_two[0]);
                exit(EXIT_FAILURE);
            }
        }
        // close pipe read
        close(pipe_two[0]);
    }
    exit(EXIT_SUCCESS);
}