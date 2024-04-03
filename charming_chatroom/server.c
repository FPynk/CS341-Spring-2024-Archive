/**
 * charming_chatroom
 * CS 341 - Spring 2024
 */
#include <arpa/inet.h>
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

#include "utils.h"

#define MAX_CLIENTS 8

void *process_client(void *p);

static volatile int serverSocket;
static volatile int endSession;

static volatile int clientsCount;
static volatile int clients[MAX_CLIENTS];

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

/**
 * Signal handler for SIGINT.
 * Used to set flag to end server.
 */
void close_server() {
    endSession = 1;
    // add any additional flags here you want.
    // Close server socket
    if (shutdown(serverSocket, SHUT_RDWR) != 0) {
        perror("Shutdown failed\n");
    }
    close(serverSocket);
    // Iterate over client sockets, shutdown and close
    for (int i = 0; i < MAX_CLIENTS; ++i) {
        if (clients[i] != -1) {
            if (shutdown(clients[i], SHUT_RDWR) != 0) {
                perror("Client shutdown failed\n");
            }  
            close(clients[i]);
        }
    }
    clientsCount = 0;
}

/**
 * Cleanup function called in main after `run_server` exits.
 * Server ending clean up (such as shutting down clients) should be handled
 * here.
 */
void cleanup() {
    if (shutdown(serverSocket, SHUT_RDWR) != 0) {
        perror("shutdown():");
    }
    close(serverSocket);

    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i] != -1) {
            if (shutdown(clients[i], SHUT_RDWR) != 0) {
                perror("shutdown(): ");
            }
            close(clients[i]);
        }
    }
}

/**
 * Sets up a server connection.
 * Does not accept more than MAX_CLIENTS connections.  If more than MAX_CLIENTS
 * clients attempts to connects, simply shuts down
 * the new client and continues accepting.
 * Per client, a thread should be created and 'process_client' should handle
 * that client.
 * Makes use of 'endSession', 'clientsCount', 'client', and 'mutex'.
 *
 * port - port server will run on.
 *
 * If any networking call fails, the appropriate error is printed and the
 * function calls exit(1):
 *    - fprtinf to stderr for getaddrinfo
 *    - perror() for any other call
 */
void run_server(char *port) {
    // Q1-3 Q8 Q4-6 Q9-11
    // Create a server socket with the right configurations
    if  ((serverSocket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        fprintf(stderr, "Failed creating client scoket\n");
        exit(EXIT_FAILURE);
    }
    // set socket options to SO_REUSEADDR and SO_REUSEPORT
    int optval = 1;
    if (setsockopt(serverSocket, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval)) < 0) {
        perror("SO_REUSEPORT failed\n");
        exit(EXIT_FAILURE);
    }
    if (setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) < 0) {
        perror("SO_REUSEADDR failed\n");
        exit(EXIT_FAILURE);
    }

    // prepare the server address and port for binding
    struct addrinfo hints;
    struct addrinfo *result;
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET; // IPV4
    hints.ai_socktype = SOCK_STREAM; // TCP connection
    hints.ai_flags = AI_PASSIVE; // use own IP
    int addr_res = 0;
    if ((addr_res = getaddrinfo(NULL, port, &hints, &result)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(addr_res));
        exit(EXIT_FAILURE);
    }

    // bind server socket to speicifed port and address
    if (bind(serverSocket, result->ai_addr, result->ai_addrlen) != 0) {
        perror("failed bind\n");
        exit(EXIT_FAILURE);
    }

    // listen
    if (listen(serverSocket, 16) < 0) {
        perror("listen failed\n");
        exit(EXIT_FAILURE);
    }

    // loop to accept and process new connections
    struct sockaddr_storage clientaddr;
    clientaddr.ss_family = AF_INET; // IPv4
    socklen_t client_addr_size = sizeof(clientaddr);
    // thread ids for each client
    pthread_t threads[MAX_CLIENTS];
    // Accept new connections
    while (endSession == 0) {
        // ensure max not reached
        if (clientsCount < MAX_CLIENTS) {
            for (int i  = 0; i < MAX_CLIENTS; ++i) {
                // find empty slot
                if (clients[i] == -1) {
                    // Attempt accept connection
                    clients[i] = accept(serverSocket, (struct sockaddr *) &clientaddr, &client_addr_size);
                    // check if failed
                    if (clients[i] == -1) {
                        perror("SERVER ACCEPT FAILED\n");
                        exit(EXIT_FAILURE);
                    }
                    // success in accepting
                    pthread_create(&threads[i], NULL, process_client, (void *) (intptr_t) i);
                    clientsCount++;
                    break; // exit loop to redo max client check
                }
            }
        }
    }
    freeaddrinfo(result);
}

/**
 * Broadcasts the message to all connected clients.
 *
 * message  - the message to send to all clients.
 * size     - length in bytes of message to send.
 */
void write_to_clients(const char *message, size_t size) {
    pthread_mutex_lock(&mutex);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i] != -1) {
            ssize_t retval = write_message_size(size, clients[i]);
            if (retval > 0) {
                retval = write_all_to_socket(clients[i], message, size);
            }
            if (retval == -1) {
                perror("write(): ");
            }
        }
    }
    pthread_mutex_unlock(&mutex);
}

/**
 * Handles the reading to and writing from clients.
 *
 * p  - (void*)intptr_t index where clients[(intptr_t)p] is the file descriptor
 * for this client.
 *
 * Return value not used.
 */
void *process_client(void *p) {
    pthread_detach(pthread_self());
    intptr_t clientId = (intptr_t)p;
    ssize_t retval = 1;
    char *buffer = NULL;

    while (retval > 0 && endSession == 0) {
        retval = get_message_size(clients[clientId]);
        if (retval > 0) {
            buffer = calloc(1, retval);
            retval = read_all_from_socket(clients[clientId], buffer, retval);
        }
        if (retval > 0)
            write_to_clients(buffer, retval);

        free(buffer);
        buffer = NULL;
    }

    printf("User %d left\n", (int)clientId);
    close(clients[clientId]);

    pthread_mutex_lock(&mutex);
    clients[clientId] = -1;
    clientsCount--;
    pthread_mutex_unlock(&mutex);

    return NULL;
}

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "%s <port>\n", argv[0]);
        return -1;
    }

    struct sigaction act;
    memset(&act, '\0', sizeof(act));
    act.sa_handler = close_server;
    if (sigaction(SIGINT, &act, NULL) < 0) {
        perror("sigaction");
        return 1;
    }

    run_server(argv[1]);
    cleanup();
    pthread_exit(NULL);
}
