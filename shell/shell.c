/**
 * shell
 * CS 341 - Spring 2024
 */
#include "format.h"
#include "shell.h"
#include "vector.h"

typedef struct process {
    char *command;
    pid_t pid;
} process;

typedef struct shell_env {
    char *history_file_path;
    char *script_file_path;
    vector *command_history;
} shell_env;

int helper_change_directory(const char *path);
int helper_history(const shell_env *env);
int helper_n(const shell_env *env, int n);
int helper_prefix(const shell_env *env, const char *prefix);

helper_change_directory() {

}

helper_history() {

}

helper_n() {

}

helper_prefix() {

}

int shell(int argc, char *argv[]) {
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
    // built-in: exe w/o creating new process
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
    // Prints out each commadn in the history in order
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

    return 0;
}
