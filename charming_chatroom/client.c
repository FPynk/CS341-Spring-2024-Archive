/**
 * charming_chatroom
 * CS 341 - Spring 2024
 */

#include <errno.h>
#include <netdb.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "chat_window.h"
#include "utils.h"

static volatile int serverSocket;
static pthread_t threads[2];

void *write_to_server(void *arg);
void *read_from_server(void *arg);
void close_program(int signal);

// dynamically allocated addr_info
static struct addrinfo *addr_structs;

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

typedef struct _thread_cancel_args {
    char **buffer;
    char **msg;
} thread_cancel_args;

/**
 * Cleanup routine in case the thread gets cancelled.
 * Ensure buffers are freed if they point to valid memory.
 */
void thread_cancellation_handler(void *arg) {
    printf("Cancellation handler\n");
    thread_cancel_args *a = (thread_cancel_args *)arg;
    char **msg = a->msg;
    char **buffer = a->buffer;
    if (*buffer) {
        free(*buffer);
        *buffer = NULL;
    }
    if (msg && *msg) {
        free(*msg);
        *msg = NULL;
    }
}

/**
 * Reads bytes from user and writes them to server.
 *
 * arg - void* casting of char* that is the username of client.
 */
void *write_to_server(void *arg) {
    char *name = (char *)arg;
    char *buffer = NULL;
    char *msg = NULL;
    ssize_t retval = 1;

    thread_cancel_args cancel_args;
    cancel_args.buffer = &buffer;
    cancel_args.msg = &msg;
    // Setup thread cancellation handlers.
    // Read up on pthread_cancel, thread cancellation states,
    // pthread_cleanup_push for more!
    pthread_cleanup_push(thread_cancellation_handler, &cancel_args);

    while (retval > 0) {
        read_message_from_screen(&buffer);
        if (buffer == NULL)
            break;

        msg = create_message(name, buffer);
        size_t len = strlen(msg) + 1;

        retval = write_message_size(len, serverSocket);
        if (retval > 0)
            retval = write_all_to_socket(serverSocket, msg, len);

        free(msg);
        msg = NULL;
    }

    pthread_cleanup_pop(0);
    return 0;
}

/**
 * Reads bytes from the server and prints them to the user.
 *
 * arg - void* requriment for pthread_create function.
 */
void *read_from_server(void *arg) {
    // Silence the unused parameter warning.
    (void)arg;
    ssize_t retval = 1;
    char *buffer = NULL;
    thread_cancel_args cancellation_args;
    cancellation_args.buffer = &buffer;
    cancellation_args.msg = NULL;
    pthread_cleanup_push(thread_cancellation_handler, &cancellation_args);

    while (retval > 0) {
        retval = get_message_size(serverSocket);
        if (retval > 0) {
            buffer = calloc(1, retval);
            retval = read_all_from_socket(serverSocket, buffer, retval);
        }
        if (retval > 0)
            write_message_to_screen("%s\n", buffer);

        free(buffer);
        buffer = NULL;
    }

    pthread_cleanup_pop(0);
    return 0;
}

/**
 * Signal handler used to close this client program.
 */
void close_program(int signal) {
    if (signal == SIGINT) {
        pthread_cancel(threads[0]);
        pthread_cancel(threads[1]);
        close_chat();
        close_server_connection();
    }
}

int main(int argc, char **argv) {
    if (argc < 4 || argc > 5) {
        fprintf(stderr, "Usage: %s <address> <port> <username> [output_file]\n",
                argv[0]);
        exit(1);
    }

    char *output_filename;
    if (argc == 5) {
        output_filename = argv[4];
    } else {
        output_filename = NULL;
    }

    // Setup signal handler.
    signal(SIGINT, close_program);
    create_windows(output_filename);
    atexit(destroy_windows);

    serverSocket = connect_to_server(argv[1], argv[2]);

    pthread_create(&threads[0], NULL, write_to_server, (void *)argv[3]);
    pthread_create(&threads[1], NULL, read_from_server, NULL);

    pthread_join(threads[0], NULL);
    pthread_join(threads[1], NULL);

    return 0;
}