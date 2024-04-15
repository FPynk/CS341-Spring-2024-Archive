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
#include <sys/stat.h>

// server imports
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>

// error import
#include <errno.h>

#include "common.h"

char **parse_args(int argc, char **argv);
verb check_args(char **args);

// Notes
// Cient request
// VERB [filename]\n // filename max 255 bytes // Max for this header is 1024 bytes
// [File size][Binary Data]
// Server Reponse
// RESPONSE\n // MAx is 1024, only expect max 6 bytes though ERROR'\n'
// [Error Message]\n // Max is 1024
// [File size][Binary Data] // [File size] is 8 bytes

// Filename limited to 255 bytes
// dynamically allocated addr_info
static struct addrinfo *addr_structs;
// File descriptor for server socket
static volatile int serverSocket;
// Size of message containing size of file
static const size_t MESSAGE_SIZE_DIGITS = 8;
#define HEADER_SIZE 1024
#define KILOBYTE 1024
#define BLOCK_SIZE KILOBYTE
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

// Reads single line from server socket
// input: buffer to readline into, size of buffer
// returns -1 error or failure; x no of bytes read does not incldue '\n'
// Note: Same as read_all_from_socket but 1 char at a time
int read_line(char *buf, size_t buf_size) {
    ssize_t b_read = 0;
    while (b_read < (ssize_t) buf_size - 1) { // leave space for null terminate
        ssize_t cur_read = read(serverSocket, buf + b_read, 1);
        if (cur_read == 0) { break; } // Done reading
        else if (cur_read > 0) { 
            char cur_char = buf[b_read];
            // Check if current char is newline
            if (cur_char == '\n') {
                buf[b_read] = '\0';
                return b_read;
            }
            b_read += cur_read; // increment bytes read
        } else if (cur_read == -1 && errno == EINTR) { continue; }// interruption, retry
        else { return -1; } // error
    }
    buf[b_read] = '\0'; // null terminte, will replace
    return b_read;
}

// Process and print appropriate error message based on the server reply
// uses format.h
// input: line corresponding to OK or ERROR
// output: 0 if OK; -1 error; 1 if anything else, invalid
int process_server_response(const char *response) {
    int status = 0;
    if (strcmp(response, "OK") == 0) {
        status = 0;
    } else if (strcmp(response, "ERROR") == 0) {
        status = -1;
        // Get error message
        char error_message[HEADER_SIZE];
        int b_read = read_line(error_message, sizeof(error_message));
        if (b_read < 0) {
            print_invalid_response();
            perror("PSR: Error then Invalid response\n");
            status = 1;
        } else {
            print_error_message(error_message);
        }
    } else {
        status = 1;
        print_invalid_response();
        perror("PSR: Invalid response\n");
    }
    return status;
}

// Request functions for various methods
// Handles GET method, return status 0 success -1 error
int GET_request(const char *remote, const char *local) {
    // format request
    char request[HEADER_SIZE];
    snprintf(request, sizeof(request), "GET %s\n", remote);
    // send to server
    ssize_t bytes_wrote = write_all_to_socket(serverSocket, request, strlen(request));
    // Check success
    if (bytes_wrote < 0) {
        perror("GET_request: Failed to write request to socket\n");
        return -1;
    }

    // close write so server processes
    shutdown(serverSocket, SHUT_WR);

    // get reply status line, note size of buffer reply is +1 of the return of read due to '\0'
    char response[6];
    if(read_line(response, sizeof(response)) < 0) {
        print_invalid_response();
        perror("GET_request: Failed to read reply_status_line from socket\n");
        return -1;
    }
    // fprintf(stderr, "response: %s\n", response);
    // Process server reply
    int status = process_server_response(response);
    if (status != 0) {
        perror("GET_request: process_server_reply ERROR\n");
        return -1;
    }
    // open file
    FILE *file = fopen(local, "w+");
    if (file == NULL) {
        perror("GET_request: failed to open file\n");
        return -1;
    }
    // set perms
    if (chmod(local, 0777) != 0) {
        perror("GET_request: failed to set file perms\n");
        return -1;
    }

    ssize_t msg_size = get_message_size(serverSocket, MESSAGE_SIZE_DIGITS);
    // fprintf(stderr, "msg_size: %ld\n", msg_size);
    if (msg_size < 0) {
        perror("GET_request: failed to get msg size\n");
        return -1;
    }
    // Stuff i need to read/write the file with
    char file_buffer[BLOCK_SIZE];
    ssize_t file_b_wrote = 0;
    ssize_t b_left_to_write = msg_size;
    // Read message in chunks, write to file
    while (file_b_wrote < msg_size) {
        b_left_to_write = msg_size - file_b_wrote;
        // read/ write in blocks
        ssize_t b_to_WR = min(BLOCK_SIZE, b_left_to_write);
        // fprintf(stderr, "b_to_WR: %ld\n", b_to_WR);
        // read from socket
        ssize_t b_read = read_all_from_socket(serverSocket, file_buffer, b_to_WR);
        if (b_read < 0) {
            perror("GET_request: failed to read msg\n");
            status = -1;
            break;
        }
        // write to file
        ssize_t b_wrote = fwrite(file_buffer, 1, b_read, file);
        if (b_wrote < b_to_WR) { // cannot be b_read
            perror("GET_request: failed to write to file\n");
            status = -1;
            break;
        }
        file_b_wrote += b_wrote;
    }
    // Check too much data
    ssize_t b_read = read_all_from_socket(serverSocket, file_buffer, 1);
    if (b_read > 0) {
        print_received_too_much_data();
        perror("GET_request: too much data\n");
        status = -1;
    }
    // Check too little data
    if (file_b_wrote < msg_size) {
        print_too_little_data();
        perror("GET_request: too little data\n");
        status = -1;
    }
    // close file
    fclose(file);
    // return status
    return status;
}

// Handles PUT method, return status 0 success -1 or 1 error
int PUT_request(const char *remote, const char *local) {
    // get file stats, size
    struct stat file_stat;
    if (stat(local, &file_stat) != 0) {
        perror("PUT_request: failed to get file stat.\n");
        exit(EXIT_FAILURE);
    }
    ssize_t file_size = file_stat.st_size;
    // open file
    FILE *file = fopen(local, "r");
    if (file == NULL) {
        perror("PUT_request: failed to open file\n");
        exit(EXIT_FAILURE);
    }
    // prep PUT request
    char request[HEADER_SIZE];
    snprintf(request, sizeof(request), "PUT %s\n", remote);
    // Send PUT request
    ssize_t bytes_wrote = write_all_to_socket(serverSocket, request, strlen(request));
    // Check success
    if (bytes_wrote < 0) {
        perror("PUT_request: Failed to write request to socket\n");
        return -1;
    }
    // send file size to server
    ssize_t bytes_wrote_size = send_message_size(serverSocket, MESSAGE_SIZE_DIGITS, file_size);
    if (bytes_wrote_size < 0) {
        perror("PUT_request: Failed to write size to socket\n");
        return -1;
    }

    // Write file contents to socket
    // Stuff i need to read/write the file with
    int status = 0;
    char file_buffer[BLOCK_SIZE];
    ssize_t msg_b_wrote = 0;
    ssize_t b_left_to_write = file_size;
    // Read message in chunks, write to file
    while (msg_b_wrote < file_size) {
        b_left_to_write = file_size - msg_b_wrote;
        // read/ write in blocks
        ssize_t b_to_WR = min(BLOCK_SIZE, b_left_to_write);
        // fprintf(stderr, "b_to_WR: %ld\n", b_to_WR);
        // read from from file
        ssize_t b_read = fread(file_buffer, 1, b_to_WR, file);
        if (b_read < b_to_WR) {
            perror("PUT_request: failed to read from file\n");
            status = -1;
            break;
        }
        // write to socket
        ssize_t b_wrote = write_all_to_socket(serverSocket, file_buffer, b_read);
        if (b_wrote < 0) {
            perror("PUT_request: failed to write msg\n");
            status = -1;
            break;
        }
        msg_b_wrote += b_wrote;
    }
    // shutdown wr to signal done
    shutdown(serverSocket, SHUT_WR);
    // close file
    fclose(file);
    // handle server response
    if (status == 0) {
        // get reply status line, note size of buffer reply is +1 of the return of read due to '\0'
        char response[6];
        if(read_line(response, sizeof(response)) < 0) {
            print_invalid_response();
            perror("PUT_request: Failed to read reply_status_line from socket\n");
            return -1;
        }
        status = process_server_response(response);
    }
    if (status == 0) {
        print_success();
    }
    // return status
    return status;
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
