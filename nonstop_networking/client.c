/**
 * nonstop_networking
 * CS 341 - Spring 2024
 */
#include "format.h"
#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

#include "common.h"

char **parse_args(int argc, char **argv);
verb check_args(char **args);

// Helpers
// Fn executes when SIGINT
void SIGINT_handler(int signal) {
    if (signal == SIGINT) {
        fprintf(stderr, "SIGINT Handler: WIP\n");
    }
}

// Request functions for various methods
// Handles GET method, return status 0 success -1 error
int GET_request(remote, local) {

}

// Handles PUT method, return status 0 success -1 error
int PUT_request(remote, local) {

}

// Handles LIST method, return status 0 success -1 error
int LIST_request() {

}

// Handles DELETE method, return status 0 success -1 error
int DELETE_request(remote) {

}

int main(int argc, char **argv) {
    // Good luck!
    check_args(argv);
    // char* array in form of {host, port, method, remote, local, NULL}
    char **args = parse_args(argc, argv); // delete
    // signal handler
    signal(SIGINT, SIGINT_handler);

    // Connect to server: args[0], args[1]
    // WIP

    // Check method
    // Store in local vars
    char *method = args[2];
    char *remote = args[3];
    char *local = args[4];
    int status = 0;

    // Dispatch
    if (strcmp(method, "GET") == 0) {
        status = GET_request(remote, local);
    } else if (strcmp(method, "PUT") == 0) {
        status = PUT_request(remote, local);
    } else if (strcmp(method, "LIST") == 0) {
        status = LIST_request();
    } else if (strcmp(method, "DELETE") == 0) {
        status = DELETE_request(remote);
    } else {
        // method not recognised
        print_client_help();
        status = 1;
    }

    // Memory management
    free(args);
    // Close connection

    return 0;
}

/**
 * Given commandline argc and argv, parses argv.
 *
 * argc argc from main()
 * argv argv from main()
 *
 * Returns char* array in form of {host, port, method, remote, local, NULL}
 * where `method` is ALL CAPS
 */
char **parse_args(int argc, char **argv) {
    if (argc < 3) {
        return NULL;
    }

    char *host = strtok(argv[1], ":");
    char *port = strtok(NULL, ":");
    if (port == NULL) {
        return NULL;
    }

    char **args = calloc(1, 6 * sizeof(char *));
    args[0] = host;
    args[1] = port;
    args[2] = argv[2];
    char *temp = args[2];
    while (*temp) {
        *temp = toupper((unsigned char)*temp);
        temp++;
    }
    if (argc > 3) {
        args[3] = argv[3];
    }
    if (argc > 4) {
        args[4] = argv[4];
    }

    return args;
}

/**
 * Validates args to program.  If `args` are not valid, help information for the
 * program is printed.
 *
 * args     arguments to parse
 *
 * Returns a verb which corresponds to the request method
 */
verb check_args(char **args) {
    if (args == NULL) {
        print_client_usage();
        exit(1);
    }

    char *command = args[2];

    if (strcmp(command, "LIST") == 0) {
        return LIST;
    }

    if (strcmp(command, "GET") == 0) {
        if (args[3] != NULL && args[4] != NULL) {
            return GET;
        }
        print_client_help();
        exit(1);
    }

    if (strcmp(command, "DELETE") == 0) {
        if (args[3] != NULL) {
            return DELETE;
        }
        print_client_help();
        exit(1);
    }

    if (strcmp(command, "PUT") == 0) {
        if (args[3] == NULL || args[4] == NULL) {
            print_client_help();
            exit(1);
        }
        return PUT;
    }

    // Not a valid Method
    print_client_help();
    exit(1);
}
