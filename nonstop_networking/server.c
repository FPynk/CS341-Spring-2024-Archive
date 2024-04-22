/**
 * nonstop_networking
 * CS 341 - Spring 2024
 */
#include "includes/vector.h"
#include "includes/dictionary.h"
#include "format.h"
#include "common.h"

#include <stdio.h>
#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/stat.h>

// server imports
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include <fcntl.h>
#include <sys/epoll.h>

// error import
#include <errno.h>

// TODO LIST:
/*
1) server_shutdown

*/ 

// constants /definitions
#define DIRECTORY_NAME "XXXXXX"     // specified by the spec
#define EVENT_LIMIT 100


// Global vars
static char *dir;                     // directory for server
static int serverSocket;              // fd for server socket
static struct addrinfo *addr_structs; // dynamically allocated addr_info
static int epoll_fd;                  // epoll file descriptor

// Function declarations
void sigpipe_handler();
void sigint_handler();
void shutdown_server();
void setup_temp_directory(char *name);
int setup_socket(char *port);
void free_addr_info();
void run_server();

// signal handlers
void sigpipe_handler() {
    // do nothing
}

// handles sigint; uses sigaction and returns if server successfully shutdown
void sigint_handler() {
    // set up struct and 0 out
    struct sigaction act;
    memset(&act, 0, sizeof(struct sigaction));
    // set function to run for SIGINT
    act.sa_handler = shutdown_server;
    if (sigaction(SIGINT, &act, NULL) < 0) {
        perror("SIGINT sigaction failed\n");
        exit(EXIT_FAILURE);
    }
}

// Shutdown server function WIP
void shutdown_server() {
    free_addr_info();
    // close epoll
    close(epoll_fd);
    // close server socket
    shutdown(serverSocket, SHUT_RDWR);
    close(serverSocket);
    // delete directory and contents

    exit(EXIT_SUCCESS);
}

// setups temp dir for server to use, accepts name as argument
void setup_temp_directory(char *name) {
    dir = malloc(strlen(name));
    strcpy(dir, name);
    dir = mkdtemp(dir);
    if (dir == NULL) {
        perror("Setup_temp_directory: Failed to create temp dir.\n");
        exit(EXIT_FAILURE);
    }
    print_temp_directory(dir);
}
// free addr structs
void free_addr_info() {
    if (addr_structs) {
        freeaddrinfo(addr_structs);
        addr_structs = NULL;
    }
} 

// set up server socket and listen
// does: socket opts, nonblocking, addr and port, bind, listen
int setup_socket(char *port) {
    // IPV4, TCP
    if ((serverSocket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("setup_socket: Failed ot create socket");
        exit(EXIT_FAILURE);
    }
    // set SO_REUSEPORT and SOREUSEADDR
    int optval = 1;             // to enable
    if (setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) < 0) {
        perror("setup_socket: Failed to setsockopt REUSEADDR.\n");
        exit(EXIT_FAILURE);
    }
    if (setsockopt(serverSocket, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval)) < 0) {
        perror("setup_socket: Failed to setsockopt REUSEPORT.\n");
        exit(EXIT_FAILURE);
    }
    // set non_blocking I/O
    int flags = fcntl(serverSocket, F_GETFL, 0);
    if (flags == -1) {
        perror("setup_socket: Failed to get flags.\n");
        exit(EXIT_FAILURE);
    }
    flags |= O_NONBLOCK;
    if (fcntl(serverSocket, F_SETFL, flags) == -1) {
        perror("setup_socket: Failed to set non-blocking.\n");
        exit(EXIT_FAILURE);
    }

    // config server socket properties
    // IPv4, TCP, wildcard IP
    struct addrinfo hints;
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    // get server addr and port
    int s = 0;
    if ((s = getaddrinfo(NULL, port, &hints, &addr_structs)) != 0) {
        fprintf(stderr, "setup_socket: getaddrinfo: %s\n", gai_strerror(s));
        shutdown_server();
        exit(EXIT_FAILURE);
    }

    // bind socket to resolved addess and port
    if (bind(serverSocket, addr_structs->ai_addr, addr_structs->ai_addrlen) != 0) {
        perror("setup_socket: Failed to bind.\n");
        shutdown_server();
        exit(EXIT_FAILURE);
    }
    // listen for connections
    if (listen(serverSocket, EVENT_LIMIT) < 0) {
        perror("setup_socket: Failed to listen.\n");
        shutdown_server();
        exit(EXIT_FAILURE);
    }
    return 0;
}

void run_server(char *port) {
    // Create socket, set socket options, getaddrinfo, bind, listen, set non-blocking I/O
    setup_socket(port);

    // Create epoll
    epoll_fd = epoll_create1(0);
    if (epoll_fd == -1) {
        perror("run_server: epoll_create1 failed");
        shutdown_server();
        exit(EXIT_FAILURE);
    }
    struct epoll_event event;
    event.events = EPOLLIN;         // monitor for input events (reading)
    event.data.fd = serverSocket;   // monitor server socket
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, serverSocket, &event) == -1) {
        perror("run_server: epoll_ctl failed");
        shutdown_server();
        exit(EXIT_FAILURE);
    }
}

int main(int argc, char **argv) {
    // check correct args passed 
    if (argc != 2) {
        print_server_usage();
        return EXIT_FAILURE;
    }

    // handle SIGPIPE to do nothing
    signal(SIGPIPE, sigpipe_handler);
    // Handle SIGINT; shutdown
    sigint_handler();

    // setup temp directory
    setup_temp_directory(DIRECTORY_NAME);

    // setup and run server; pass port
    run_server(argv[1]);
    // cleanup mem?

    return EXIT_SUCCESS;
}
