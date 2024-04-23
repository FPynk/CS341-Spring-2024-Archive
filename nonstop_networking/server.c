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
#include <dirent.h>

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
#define MAX_FILENAME_LENGTH 255
#define HEADER_SIZE 1024
#define KILOBYTE 1024
#define BLOCK_SIZE 4 * KILOBYTE
#define ERROR "ERROR"
#define OK "OK"
#define NO_MSG ""
#define HTTPS_BAD_REQUEST "400"
#define HTTPS_NOT_FOUND "404"
// #define INVALID_FILE_SIZE "Too much or too little data in file"
#define FAILED_WRITE "Failed to write to file on server"

// Global vars
static char *dir;                     // directory for server
static int serverSocket;              // fd for server socket
static struct addrinfo *addr_structs; // dynamically allocated addr_info
static int epoll_fd;                  // epoll file descriptor
static const size_t MESSAGE_SIZE_DIGITS = 8;

// Function declarations
void sigpipe_handler();
void sigint_handler();
int remove_directory(const char *path);
void shutdown_server();
int read_line(int socket, char *buf, size_t buf_size);
int send_response(const char *response_type, const char *msg, int client_fd);
void setup_temp_directory(char *name);
int setup_socket(char *port);
void free_addr_info();
int set_non_blocking(int socket);
int accept_new_client(struct sockaddr_storage clientaddr, socklen_t client_addr_size, struct epoll_event event);
int LIST_request(int client_fd);
int GET_request(int client_fd, char *filename);
int PUT_request(int client_fd, char *filename);
int DELETE_request(int client_fd, char *filename);
int process_client_reponse(int client_fd);
int process_epoll_events(struct sockaddr_storage clientaddr, socklen_t client_addr_size,
                         int n_fds, struct epoll_event event, struct epoll_event *events);
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

int remove_directory(const char *path) {
    DIR *del_dir = opendir(path);
    if (del_dir == NULL) {
        perror("shutdown_server: failed to open directory");
        return -1;
    }
    struct dirent *entry;
    // keep track of result of deletion
    int r = 0;
    while ((entry = readdir(del_dir)) != NULL) {
        char full_path[HEADER_SIZE];
        // parent/ cur directory
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        // format path
        snprintf(full_path, sizeof(full_path), "%s/%s", path, entry->d_name);
        // edit stat file
        struct stat statbuf;
        if (stat(full_path, &statbuf) == -1) {
            perror("Failed to gget file status");
            r = -1;
            break;
        }
        if (S_ISDIR(statbuf.st_mode)) {
            r = remove_directory(full_path);
        } else {
            r = unlink(full_path);
        }
        if (r == -1) {
            perror("Error deleting file or direcotry.\n");
            break;
        }
    }
    closedir(del_dir);
    if (r == 0) {
        r = rmdir(path);
        if (r == -1) {
            perror("Error removing directory.\n");
        }
    }
    return r;
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
    if (remove_directory(dir) == -1) {
        perror("Shutdown_server: failed to remove directory.\n");
        exit(EXIT_FAILURE);
    }
    free(dir);
    exit(EXIT_SUCCESS);
}

// Reads single line from server socket
// input: buffer to readline into, size of buffer
// returns -1 error or failure; x no of bytes read does not incldue '\n'
// Note: Same as read_all_from_socket but 1 char at a time
int read_line(int socket, char *buf, size_t buf_size) {
    ssize_t b_read = 0;
    while (b_read < (ssize_t) buf_size) {
        ssize_t cur_read = read(socket, buf + b_read, 1);
        if (cur_read == 0) { break; } // Done reading
        else if (cur_read > 0) { 
            char cur_char = buf[b_read];
            // fprintf(stderr, "Read line char: %c\n", cur_char);
            // Check if current char is newline
            if (cur_char == '\n') {
                // fprintf(stderr, "Read line char is newline\n");
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

// Sends reponse back to client
int send_response(const char *response_type, const char *msg, int client_fd) {
    char response[HEADER_SIZE];
    if (strcmp(response_type, ERROR) == 0) {
        snprintf(response, sizeof(response), "%s\n%s\n", response_type, msg);
    } else {
        snprintf(response, sizeof(response), "%s\n", response_type);
    }
    return write_all_to_socket(client_fd, response, strlen(response));
}

// setups temp dir for server to use, accepts name as argument
void setup_temp_directory(char *name) {
    dir = malloc(sizeof(name));
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
// set non blocking I/O for the socket
int set_non_blocking(int socket) {
    int flags = fcntl(socket, F_GETFL, 0);
    if (flags == -1) {
        perror("set_non_blocking: Failed to get flags.\n");
        return EXIT_FAILURE;
    }
    flags |= O_NONBLOCK;
    if (fcntl(socket, F_SETFL, flags) == -1) {
        perror("set_non_blocking: Failed to set non-blocking.\n");
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
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
    if (set_non_blocking(serverSocket) != 0) {
        perror("setup_socket: Failed to set_non_blocking.\n");
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

// Accepts new client from server socket, add to epoll_fd
// returns 
int accept_new_client(struct sockaddr_storage clientaddr, socklen_t client_addr_size, struct epoll_event event) {
    // serverSocket event; accept
    int result = 0;
    // Continue accepting while accept has clients to accept
    while(1) {
        int client_fd = accept(serverSocket, (struct sockaddr *) &clientaddr, &client_addr_size);
        // Client error
        if (client_fd == -1) {
            // check other connections
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                // all connections processed, break out
                break;
            } else {
                perror("accept_new_client: Error accepting client.\n");
                continue;
            }   
        }
        // Set nonblocking I/O
        if (set_non_blocking(client_fd) < 0) {
            perror("accept_new_client: error nonblocking I/o.\n");
            close(client_fd);
            continue;
        }
        // add to epoll
        // set client_fd event info; edge trigger try
        event.data.fd = client_fd;
        event.events = EPOLLIN | EPOLLET;
        if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &event) == -1) {
            perror("accept_new_client: epoll_ctl failed.\n");
            close(client_fd);
            continue;
        }
        ++result;
    }
    return result;
}

int LIST_request(int client_fd) {
    // send ok response
    send_response(OK, NO_MSG, client_fd);

    
    return 0;
}

int GET_request(int client_fd, char *filename) {
    // contruct path to file
    char path[strlen(dir) + strlen(filename) + 2];
    snprintf(path, sizeof(path), "%s/%s", dir, filename);
    // get file stats, size
    struct stat file_stat;
    if (stat(path, &file_stat) != 0) {
        perror("GET_request: failed to get file stat.\n");
        send_response(ERROR, HTTPS_NOT_FOUND, client_fd);
        return EXIT_FAILURE;
    }
    ssize_t file_size = file_stat.st_size;
    // open file
    FILE *file = fopen(path, "r");
    if (file == NULL) {
        perror("GET_request: failed to open file\n");
        send_response(ERROR, HTTPS_NOT_FOUND, client_fd);
        return EXIT_FAILURE;
    }
    // send response
    // RESPONSE\n
    // [Error Message]\n
    // [File size][Binary Data]
    if (send_response(OK, NO_MSG, client_fd) < 0) {
        return EXIT_FAILURE;
    }
    if (send_message_size(client_fd, MESSAGE_SIZE_DIGITS, file_size) < 0) {
        return EXIT_FAILURE;
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
            perror("GET_request: failed to read from file\n");
            status = -1;
            break;
        }
        // write to socket
        ssize_t b_wrote = write_all_to_socket(client_fd, file_buffer, b_read);
        if (b_wrote < 0) {
            perror("GET_request: failed to write msg\n");
            status = -1;
            break;
        }
        msg_b_wrote += b_wrote;
    }
    // did not shutdown, done ltr
    // close file
    fclose(file);
    return status;
}

int PUT_request(int client_fd, char *filename) {
    // contruct path to file
    char path[strlen(dir) + strlen(filename) + 2];
    snprintf(path, sizeof(path), "%s/%s", dir, filename);
    // open file
    FILE *file = fopen(path, "w+");
    if (file == NULL) {
        perror("PUT_request: failed to open file\n");
        send_response(ERROR, HTTPS_NOT_FOUND, client_fd);
        return EXIT_FAILURE;
    }
    ssize_t msg_size = get_message_size(client_fd, MESSAGE_SIZE_DIGITS);
    if (msg_size < 0) {
        perror("PUT_request: failed get size\n");
        send_response(ERROR, HTTPS_BAD_REQUEST, client_fd);
        return EXIT_FAILURE;
    }
    // fprintf(stderr, "msgsize: %ld\n", msg_size);
    // Stuff i need to read/write the file with
    int status = 0;
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
        ssize_t b_read = read_all_from_socket(client_fd, file_buffer, b_to_WR);
        if (b_read < 0) {
            perror("PUT_request: failed to read msg\n");
            status = -1;
            break;
        }
        // write to file
        ssize_t b_wrote = fwrite(file_buffer, 1, b_read, file);
        if (b_wrote < b_to_WR) { // cannot be b_read
            perror("PUT_request: failed to write to file\n");
            status = -1;
            break;
        }
        file_b_wrote += b_wrote;
    }
    // close file
    fclose(file);
    // check file sizes
    // Check too much data
    ssize_t b_read = read_all_from_socket(serverSocket, file_buffer, 1);
    if (b_read > 0) {
        perror("PUT_request: too much data\n");
        status = -1;
    }
    // Check too little data
    if (file_b_wrote < msg_size) {
        perror("PUT_request: too little data\n");
        status = -1;
    }
    // send ERROR response if status -1
    if (status == -1) {
        perror("PUT_request: status -1\n");
        send_response(ERROR, FAILED_WRITE, client_fd);
        return EXIT_FAILURE;
    }
    // Send OK response
    if (send_response(OK, NO_MSG, client_fd) < 0) {
        return EXIT_FAILURE;
    }
    return status;
}

int DELETE_request(int client_fd, char *filename) {
    // contruct path to file
    char path[strlen(dir) + strlen(filename) + 2];
    snprintf(path, sizeof(path), "%s/%s", dir, filename);

    // open file
    FILE *file = fopen(path, "r");
    if (file == NULL) {
        perror("DELETE_request: failed to open file\n");
        send_response(ERROR, HTTPS_NOT_FOUND, client_fd);
        return EXIT_FAILURE;
    }
    fclose(file);
    if (unlink(path) != 0) {
        perror("DELETE_request: failed to delete file\n");
        send_response(ERROR, HTTPS_NOT_FOUND, client_fd);
        return EXIT_FAILURE;
    }
    send_response(OK, NO_MSG, client_fd);
    return 0;
}

int process_client_reponse(int client_fd) {
    // Get header
    char request[HEADER_SIZE];
    if (read_line(client_fd, request, sizeof(request)) < 0) {
        print_invalid_response();
        perror("process_client_reponse: could not read line.\n");
        return EXIT_FAILURE;
    }
    // Parse VERB
    char *space_pos = strchr(request, ' ');
    if (!space_pos && strcmp(request, "LIST") == 0) {
        // handle LIST
        LIST_request(client_fd);
    } else if (space_pos) {
        // Check GET, PUT or DEL
        // split request line in 2
        *space_pos = '\0';
        char *filename = space_pos + 1;
        char *verb = request;
        // check filename length
        if (strlen(filename) > MAX_FILENAME_LENGTH) {
            print_invalid_response();
            perror("process_client_reponse: MAX_FILENAME_LENGTH exceeded.\n");
            send_response(ERROR, HTTPS_BAD_REQUEST, client_fd);
            return EXIT_FAILURE;
        }
        // dispatch
        if (strcmp(verb, "GET") == 0) {
            GET_request(client_fd, filename);
        } else if (strcmp(verb, "PUT") == 0) {
            PUT_request(client_fd, filename);
        } else if (strcmp(verb, "DELETE") == 0) {
            DELETE_request(client_fd, filename);
        } else {
            print_invalid_response();
            perror("process_client_reponse: unknown verb.\n");
            send_response(ERROR, HTTPS_BAD_REQUEST, client_fd);
            return EXIT_FAILURE;
        }
    } else {
        print_invalid_response();
        perror("process_client_reponse: unknown request.\n");
        send_response(ERROR, HTTPS_BAD_REQUEST, client_fd);
        return EXIT_FAILURE;
    }
    return 0;
}

int process_epoll_events(struct sockaddr_storage clientaddr, socklen_t client_addr_size,
                         int n_fds, struct epoll_event event, struct epoll_event *events) {
    // cycle and process each event
    for (int i = 0; i < n_fds; ++i) {
        if (events[i].data.fd == serverSocket) {
            // serverSocket event; accept
            accept_new_client(clientaddr, client_addr_size, event);
        } else {
            // client event, process and close
            int client_fd = events[i].data.fd;
            if (events[i].events & EPOLLIN) {
                // epoll as expected
                process_client_reponse(client_fd);
                // close client_fd and remove from monitoring
                shutdown(client_fd, SHUT_RDWR);
                close(client_fd);
                epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client_fd, NULL);
            } else if (events[i].events & EPOLLHUP || events[i].events & EPOLLERR) {
                // Epoll fd error
                perror("run_server: EPOLLHUP || EPOLERR.\n");
                close(client_fd);
                epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client_fd, NULL);
            }
        }
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
        /* epoll notes:
    if from serverSocket, connect client, add to epoll_fd
    if from client, check status and exe accordingly
    if client request complete, remove from epoll_fd
    */
    // use this to track fds to monitor
    struct epoll_event event;
    event.events = EPOLLIN;         // monitor for input events (reading)
    event.data.fd = serverSocket;   // monitor server socket
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, serverSocket, &event) == -1) {
        perror("run_server: epoll_ctl failed");
        shutdown_server();
        exit(EXIT_FAILURE);
    }
    // events we need to handle
    struct epoll_event events[EVENT_LIMIT];
    int n_fds = 0;
    // store client info
    struct sockaddr_storage clientaddr;
    clientaddr.ss_family = AF_INET;
    socklen_t client_addr_size = sizeof(clientaddr);

    // main loop to proccess server stuff
    while(1) {
        // wait for events
        // fprintf(stderr, "epoll_wait\n");
        n_fds = epoll_wait(epoll_fd, events, EVENT_LIMIT, -1);
        // fprintf(stderr, "n_fds: %d\n", n_fds);
        if (n_fds < 0) {
            perror("run_server: epoll_wait error.\n");
            shutdown_server();
            exit(EXIT_FAILURE);
        }

        // Process events
        process_epoll_events(clientaddr, client_addr_size, n_fds, event, events);
    }
    shutdown_server();
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
