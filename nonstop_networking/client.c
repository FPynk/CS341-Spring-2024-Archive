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

// server imports
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>

#include "common.h"

char **parse_args(int argc, char **argv);
verb check_args(char **args);

// dynamically allocated addr_info
static struct addrinfo *addr_structs;
// File descriptor for server socket
static volatile int serverSocket;

// Helpers
// free addr_structs
void free_addr_info() {
    if (addr_structs) {
        freeaddrinfo(addr_structs);
        addr_structs = NULL;
    }
}

/**
 * Shuts down connection with 'serverSocket'.
 * Called by close_program upon SIGINT.
 */
void close_server_connection() {
    // Your code here
    // Check addr info allocated, free if so
    if (addr_structs) {
        freeaddrinfo(addr_structs);
        addr_structs = NULL;
    }
    // Shutdwon read part of socket to stop read ops
    shutdown(serverSocket, SHUT_RD);
    // close server socket
    close(serverSocket);
}


/**
 * Sets up a connection to a chatroom server and returns
 * the file descriptor associated with the connection.
 *
 * host - Server to connect to.
 * port - Port to connect to server on.
 *
 * Returns integer of valid file descriptor, or exit(1) on failure.
 */
int connect_to_server(const char *host, const char *port) {
    // Q1-7
    // Note: Need to use IPv4 TCP
    struct addrinfo c_info;
    // socket file descriptor for client
    int sockfd;
    // initialise c_info to zero and set desired properties
    memset(&c_info, 0, sizeof(struct addrinfo));
    c_info.ai_family = AF_INET; // IPv4
    c_info.ai_socktype = SOCK_STREAM; // TCP
    c_info.ai_protocol = 0;

    // get list of addr structures for the server
    int addr_res;
    if ((addr_res = getaddrinfo(host, port, &c_info, &addr_structs)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(addr_res));
        free_addr_info();
        exit(EXIT_FAILURE);
    }
    // Create client socket
    if ((sockfd = socket(c_info.ai_family, c_info.ai_socktype, c_info.ai_protocol)) < 0) {
        perror("Cannot create client socket");
        free_addr_info();
        exit(EXIT_FAILURE);
    }
    // Attempt connection using the obtained addr structs
    struct addrinfo *temp;
    for (temp = addr_structs; temp != NULL; temp = temp->ai_next) {
        if (connect(sockfd, temp->ai_addr, temp->ai_addrlen) != -1) {
            // Successful connection, stop trying
            break;
        }
    }

    // Check connection successful
    if (!temp) {
        perror("Cannot connect to server");
        free_addr_info();
        exit(EXIT_FAILURE);
    }

    // Mem handle
    free_addr_info();
    return sockfd;
}


// Request functions for various methods
// Handles GET method, return status 0 success -1 error
int GET_request(const char *remote, const char *local) {
    return 0;
}

// Handles PUT method, return status 0 success -1 error
int PUT_request(const char *remote, const char *local) {
    return 0;
}

// Handles LIST method, return status 0 success -1 error
int LIST_request() {
    return 0;
}

// Handles DELETE method, return status 0 success -1 error
int DELETE_request(const char *remote) {
    return 0;
}

int main(int argc, char **argv) {
    // Good luck!
    check_args(argv);
    // char* array in form of {host, port, method, remote, local, NULL}
    char **args = parse_args(argc, argv); // delete

    // Connect to server: args[0], args[1]
    serverSocket = connect_to_server(args[0], args[1]);

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
    fprintf(stderr, "status: %d\n", status);
    // Memory management
    free(args);
    // Close connection
    close_server_connection();
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
