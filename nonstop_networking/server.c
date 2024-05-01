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
#include <time.h>

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
Dictionary of client fd to info
if interrrupted, get stage it was at, then resume stage
Shutdown: server, handle the dictionary memory
*/ 

// Current status
/*
Right now solved the issue of client sending slower than server
basically keep reading when -1 and only break 0 since 0 is when connection closed
however this is inefficient
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
#define HTTPS_BAD_REQUEST err_bad_request
#define HTTPS_NOT_FOUND err_no_such_file
#define MAX_RETRY 20
// #define INVALID_FILE_SIZE "Too much or too little data in file"
#define BAD_FILE_SIZE err_bad_file_size

// Global vars
static char *dir;                     // directory for server
static int serverSocket;              // fd for server socket
static struct addrinfo *addr_structs; // dynamically allocated addr_info
static int epoll_fd;                  // epoll file descriptor
static const size_t MESSAGE_SIZE_DIGITS = 8;
static dictionary *client_dictionary;

// enums for stage
typedef enum {
    UNINITIALISED, // first set up
    OTHER,  // phase this out
    PARSE, // 2
    START, // 3
    FILE_CHECK, // 4
    RESPONSE, //5
    MSG_SIZE_RDWR, // 6
    RDWR_LOOP, // 7
    FINAL_CHECKS, // 8
    DONE, // 9
} Stage;

// client_info struct
typedef struct client_info {
    int client_fd;
    verb verb_;
    char filename[MAX_FILENAME_LENGTH + 1];  
    size_t filename_length; // length of filename
    ssize_t msg_size;       // original msg_ize
    ssize_t bytes_processed;// bytes written
    Stage stage;            // stage its in, currently only supports jumping back mid PUT loop
    int status;             // status, should be 0 if OK, 2 if interrupted, 1 and -1 likely error
} client_info;

// Function declarations
void sleep_nano(ssize_t time);
void sigpipe_handler();
void sigint_handler();
int remove_directory(const char *path);
client_info *init_empty_client_info();
void delete_remove_dictionary_entry(int client_fd);
void print_client_info(int client_fd);
void shutdown_server();
int read_line(int socket, char *buf, size_t buf_size);
int send_response(const char *response_type, const char *msg, int client_fd);
void setup_temp_directory(char *name);
int setup_socket(char *port);
void free_addr_info();
int set_non_blocking(int socket);
int accept_new_client(struct sockaddr_storage clientaddr, socklen_t client_addr_size,
                     struct epoll_event event);
// int put_read_socket_write_file(client_info *current_client_info);
int LIST_request(int client_fd);
int GET_request_dynamic(int client_fd);
// int GET_request(int client_fd, char *filename);
int PUT_request_dynamic(int client_fd);
// int PUT_request(int client_fd, char *filename);
int DELETE_request(int client_fd, char *filename);
int process_client_request_dynamic(int client_fd);
// int process_client_reponse(int client_fd);
// int process_epoll_events(struct sockaddr_storage clientaddr, socklen_t client_addr_size,
//                          int n_fds, struct epoll_event event, struct epoll_event *events);
int process_epoll_events_dynamic(struct sockaddr_storage clientaddr, socklen_t client_addr_size,
                                int n_fds, struct epoll_event event, struct epoll_event *events);
void run_server();

// need this for weird bugs
void sleep_nano(ssize_t time) {
    struct timespec ts;
    ts.tv_sec = 0;             // Seconds
    ts.tv_nsec = time;             // Nanoseconds (1 millisecond = 1,000,000 nanoseconds)
    nanosleep(&ts, NULL);
}

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

// Inits blank client info, allocated on heap
client_info *init_empty_client_info() {
    client_info *client_fd_info = malloc(sizeof(client_info));
    if (client_fd_info == NULL) {
        perror("init_empty_client_info: Failed to malloc\n");
        return NULL;
    }
    client_fd_info->client_fd = 0;
    client_fd_info->verb_ = V_UNKNOWN;
    client_fd_info->filename[0] = '\0';
    client_fd_info->filename_length = 0;
    client_fd_info->msg_size = 0;
    client_fd_info->bytes_processed = 0;
    client_fd_info->stage = UNINITIALISED;
    client_fd_info->status = 0;
    return client_fd_info;
}

void delete_remove_dictionary_entry(int client_fd) {
    client_info *current_client_info = dictionary_get(client_dictionary, &client_fd);
    if (current_client_info) {
        free(current_client_info);
        dictionary_remove(client_dictionary, &client_fd);
    } else {
        perror("delete_remove_dictionary_entry: entry DNE");
    }
}

void print_client_info(int client_fd) {
    // client_info *client_fd_info = dictionary_get(client_dictionary, &client_fd);
    // if (client_fd_info) {
    //     fprintf(stderr, "\nClient details: client_fd: %d verb: %d \nfilename: %s \nfilename_length: %ld msg_size: %ld \nbytes_processed: %ld stage: %d status: %d\n\n",
    //     client_fd_info->client_fd, client_fd_info->verb_, client_fd_info->filename,
    //     client_fd_info->filename_length, client_fd_info->msg_size,
    //     client_fd_info->bytes_processed, client_fd_info->stage, client_fd_info->status);
    // } else {
    //     perror("print_client_info: entry DNE");
    // }
}

// Shutdown server function WIP
void shutdown_server() {
    free_addr_info();
    // close epoll
    close(epoll_fd);
    // close server socket
    shutdown(serverSocket, SHUT_RDWR);
    close(serverSocket);
    if (client_dictionary) {
        // TODO ERASE DICITONARY CONTENTS
        vector *client_dictionary_keys = dictionary_keys(client_dictionary);
        for (size_t i = 0; i < vector_size(client_dictionary_keys); ++i) {
            int key = *((int *) vector_get(client_dictionary_keys, i));
            delete_remove_dictionary_entry(key);
        }
        vector_destroy(client_dictionary_keys);
        // WILL BE SHALLOW
        dictionary_destroy(client_dictionary);
    }
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
    fprintf(stderr, "Sent Response: %s", response);
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
    // fprintf(stderr, "Set fd to nonblocking: %d\n", socket);
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
                fprintf(stderr, "NO ACCEPTS LEFT client_fd: %d\n", client_fd);
                break;
            } else {
                perror("accept_new_client: Error accepting client.\n");
                continue;
            }   
        }
        // fprintf(stderr, "Accepted client_fd: %d\n", client_fd);
        // Set nonblocking I/O
        if (set_non_blocking(client_fd) < 0) {
            perror("accept_new_client: error nonblocking I/o.\n");
            close(client_fd);
            continue;
        }
        // add to epoll
        // set client_fd event info; edge trigger try
        fprintf(stderr, "Added to event client_fd: %d\n", client_fd);
        // struct epoll_event client_event;
        // memset(&client_event, 0, sizeof(client_event));
        // client_event.events = EPOLLIN | EPOLLET;         // monitor for input events (reading)
        // client_event.data.fd = client_fd;
        // if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &client_event) == -1) {
        //     perror("accept_new_client: epoll_ctl failed.\n");
        //     close(client_fd);
        //     continue;
        // }
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
    // Grab dictionary entry, shallow copy is reference
    client_info *current_client_info = dictionary_get(client_dictionary, &client_fd);
    // Set Verb type and stage
    current_client_info->verb_ = LIST;
    current_client_info->stage = OTHER;

    // send ok response
    fprintf(stderr, "LIST_request\n");
    send_response(OK, NO_MSG, client_fd);

    // open directory
    DIR *read_dir = opendir(dir);
    if (read_dir == NULL) {
        perror("LIST_request: failed to open directory");
        return -1;
    }

    struct dirent *entry;
    // Collect file names in vector
    vector *file_names = string_vector_create();
    size_t message_size = 0;

    while ((entry = readdir(read_dir)) != NULL) {
        // parent/ cur directory
        char *file_name = entry->d_name;
        if (strcmp(file_name, ".") != 0 && strcmp(file_name, "..") != 0) {
            fprintf(stderr, "file name is: %s\n", file_name);
            vector_push_back(file_names, file_name);
            message_size += strlen(file_name) + 1; // include \0 so we can put \n
        }
    }
    // close dir
    closedir(read_dir);
    // adjust size
    if (message_size > 0) {
        message_size--; // remove last \n
    }

    // Dictionary adjustment
    current_client_info->msg_size = message_size;
    current_client_info->stage = RDWR_LOOP;

    // send message size
    send_message_size(client_fd, MESSAGE_SIZE_DIGITS, message_size);
    size_t total_files = vector_size(file_names);
    int status = 0;
    // send filenames + check for success
    for (size_t i = 0; i < total_files; ++i) {
        char *file_name = (char *) vector_get(file_names, i);
        size_t bytes_sent = write_all_to_socket(client_fd, file_name, strlen(file_name));
        if (bytes_sent < 0) {
            perror("LIST_request: Failed to send file_name\n");
            status = EXIT_FAILURE;
            break;
        }
        if (i < total_files - 1) {
            bytes_sent = write_all_to_socket(client_fd, "\n", 1);
        }
        if (bytes_sent < 0) {
            perror("LIST_request: Failed to send newline\n");
            status = EXIT_FAILURE;
            break;
        }
        // Dictionary: update bytes processed
        current_client_info->bytes_processed += bytes_sent;
    }
    // Update dictionary
    current_client_info->stage = DONE;
    // manage memory
    vector_destroy(file_names);
    return status;
}

int GET_request_dynamic(int client_fd) {
    // Grab dictionary entry, shallow copy is reference
    client_info *current_client_info = dictionary_get(client_dictionary, &client_fd);
    char *filename = current_client_info->filename;
    int status = current_client_info->status;

    // contruct path to file
    char path[strlen(dir) + strlen(filename) + 2];
    snprintf(path, sizeof(path), "%s/%s", dir, filename);

    FILE *file = NULL;
    if (current_client_info->stage == START || current_client_info->stage == FILE_CHECK) {
        // get file stats, size
        struct stat file_stat;
        if (stat(path, &file_stat) != 0) {
            perror("GET_dynamic: failed to get file stat.\n");
            send_response(ERROR, HTTPS_NOT_FOUND, client_fd);
            return EXIT_FAILURE;
        }
        ssize_t file_size = file_stat.st_size;
        // update client filesize
        current_client_info->msg_size = file_size;
        // open file
        file = fopen(path, "r");
        if (file == NULL) {
            perror("GET_dynamic: failed to open file\n");
            send_response(ERROR, HTTPS_NOT_FOUND, client_fd);
            return EXIT_FAILURE;
        }
        // update stage
        current_client_info->stage = RESPONSE;
    }
    // send response
    // RESPONSE\n
    // [Error Message]\n
    // [File size][Binary Data]
    // The nesting here is intense
    if (current_client_info->stage == RESPONSE) {
        int result = send_response(OK, NO_MSG, client_fd);
        if (result < 0 && errno == EAGAIN) {
            perror("GET_dynamic, send response failed TRY AGAIN\n");
            return 2; // try again
        } else if (result < 0) {
            perror("GET_dynamic, send response failed EXIT\n");
            return EXIT_FAILURE; // failure
        }
        // success
        current_client_info->stage = MSG_SIZE_RDWR;
    }
    if (current_client_info->stage == MSG_SIZE_RDWR) {
        int result = send_message_size(client_fd, MESSAGE_SIZE_DIGITS, current_client_info->msg_size);
        if (result < 0 && errno == EAGAIN) {
            perror("GET_dynamic, send response failed TRY AGAIN\n");
            return 2; // try again
        } else if (result < 0) {
            perror("GET_dynamic, send msg_size failed EXIT\n");
            return EXIT_FAILURE; // failure
        }
        current_client_info->stage = RDWR_LOOP;
    }

    if (current_client_info->stage == RDWR_LOOP) {
        ssize_t file_size = current_client_info->msg_size;
        // open file if null in append: redoing sending
        if (!file) {
            fprintf(stderr, "Opening file in READ\n");
            // open file in append, since file already created
            file = fopen(path, "r");
            if (file == NULL) {
                perror("GET_dynamic: failed to open file\n");
                send_response(ERROR, HTTPS_NOT_FOUND, client_fd);
                return EXIT_FAILURE;
            }
            // offset correctly
            if (fseek(file, current_client_info->bytes_processed, SEEK_SET) != 0) {
                perror("GET_dynamic: Error seeking\n");
                fclose(file);
                return 1;
            }
        }

        // reset status to 0
        status = 0;

        // Write file contents to socket
        // Stuff i need to read/write the file with
        char file_buffer[BLOCK_SIZE];
        ssize_t msg_b_wrote = current_client_info->bytes_processed;
        ssize_t b_left_to_write = file_size - msg_b_wrote;
        // Read message in chunks, write to file
        while (msg_b_wrote < file_size) {
            b_left_to_write = file_size - msg_b_wrote;
            // read/ write in blocks
            ssize_t b_to_WR = min(BLOCK_SIZE, b_left_to_write);
            // fprintf(stderr, "b_to_WR: %ld\n", b_to_WR);
            // read from from file
            ssize_t b_read = fread(file_buffer, 1, b_to_WR, file);
            if (b_read < b_to_WR) {
                perror("GET_dynamic: failed to read from file\n");
                status = -1;
                break;
            }
            // BEWARE SPHAGET: b_wrote is more like status now tbh
            // write to socket
            // b_write for this while loop, b wrote for outer while loop
            ssize_t b_write = 0;
            ssize_t b_wrote = 0;
            while (b_write < (ssize_t) b_read) {
                ssize_t cur_write = write(client_fd, file_buffer + b_write, b_read - b_write);
                if (cur_write == 0) { break; } // done
                else if (cur_write > 0) { 
                    // increment
                    b_write += cur_write; 
                    b_wrote = b_write;
                }
                else if (cur_write == -1 && errno == EINTR) { continue; } //interrupt, retry
                else { 
                    b_wrote = -1;
                    break; 
                } // error, could be EAGAIN, still need update write
            }
            msg_b_wrote += b_write; // update regardless
            // update dictionary
            current_client_info->bytes_processed = msg_b_wrote;
            // ssize_t b_wrote = write_all_to_socket(client_fd, file_buffer, b_read);
            if (b_wrote < 0) {
                // error check errno for details
                // perror("GET_dynamic: failed to write msg\n");
                status = -1;
                if (errno == EAGAIN) {
                    // perror("EAGAIN, Blocks to write, TRY AGAIN\n");
                    status = 2;
                }
                break;
            } else if (b_wrote == 0) {
                // connection closed, need to check data
                perror("GET_dynamic: EOF/Connection closed\n");
                status = 0;
                break;
            }
        }
        // update dictionary
        if (status == 0) {
            // success, status == 0
            current_client_info->stage = DONE;
        }
    }
    // did not shutdown, done ltr
    // close file
    if (file) {
        fclose(file);
    }
    return status;
}

int DELETE_request(int client_fd, char *filename) {
    // Grab dictionary entry, shallow copy is reference
    client_info *current_client_info = dictionary_get(client_dictionary, &client_fd);
    // Set Verb type and stage
    current_client_info->verb_ = DELETE;
    current_client_info->stage = OTHER;

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
    // update client
    current_client_info->stage = DONE;
    return 0;
}

int PUT_request_dynamic(int client_fd) {
    fprintf(stderr, "PUT_dynamic running\n");
    // Grab dictionary entry, shallow copy is reference
    client_info *current_client_info = dictionary_get(client_dictionary, &client_fd);
    char *filename = current_client_info->filename;
    int status = current_client_info->status;
    
    print_client_info(current_client_info->client_fd);

    // contruct path to file
    char path[strlen(dir) + strlen(filename) + 2];
    snprintf(path, sizeof(path), "%s/%s", dir, filename);
    fprintf(stderr, "PUT: %s\n", filename);

    FILE *file = NULL;
    if (current_client_info->stage == START || current_client_info->stage == FILE_CHECK) {
        current_client_info->stage = FILE_CHECK;
        print_client_info(current_client_info->client_fd);
        // open file
        file = fopen(path, "w+");
        if (file == NULL) {
            perror("PUT_dynamic: failed to open file\n");
            send_response(ERROR, HTTPS_NOT_FOUND, client_fd);
            return EXIT_FAILURE;
        }
        // update stage to next one
        current_client_info->stage = MSG_SIZE_RDWR;
    }
    // might need to deocmpose get_message_size for more control
    if (current_client_info->stage == MSG_SIZE_RDWR) {
        print_client_info(current_client_info->client_fd);
        ssize_t msg_size = get_message_size(client_fd, MESSAGE_SIZE_DIGITS);
        // check if actual error or just no data atm
        if (msg_size < 0 && errno == EAGAIN) {
            perror("PUT_dynamic: failed get size\n");
            // send_response(ERROR, HTTPS_BAD_REQUEST, client_fd);
            // check and close file
            if (file) { fclose(file); }
            return 2; // try to read msg size again
        } else if (msg_size < 0) { // may be get 0 bytes file idk
            // -1
            if (file) {
                fclose(file);
            }
            send_response(ERROR, HTTPS_BAD_REQUEST, client_fd);
            return -1;
        }
        // fprintf(stderr, "msg_size: %ld\n", msg_size);
        // update client info
        sleep_nano(100); // DO NOT REMOVE, WILL BREAK CODE AND SEGFAULT IDK WHY
        // fprintf(stderr, "CCI_msg_size: %p\n", &current_client_info->msg_size);
        current_client_info->msg_size = msg_size;
        // update client stage
        current_client_info->stage = RDWR_LOOP;
    }
    
    if (current_client_info->stage == RDWR_LOOP) {
        // fprintf(stderr, "RDWR_LOOP\n");
        print_client_info(current_client_info->client_fd);
        // open file if null
        if (!file) {
            // fprintf(stderr, "Opening file in APPEND\n");
            // open file in append, since file already created
            file = fopen(path, "a");
            if (file == NULL) {
                perror("PUT_dynamic: failed to open file\n");
                send_response(ERROR, HTTPS_NOT_FOUND, client_fd);
                return EXIT_FAILURE;
            }
        }

        ssize_t msg_size = current_client_info->msg_size;
        // // fprintf(stderr, "msgsize: %ld\n", msg_size);
        // // Stuff i need to read/write the file with
        status = 0;
        char file_buffer[BLOCK_SIZE];
        // dynamic adjustment
        ssize_t file_b_wrote = current_client_info->bytes_processed;
        ssize_t b_left_to_write = msg_size - file_b_wrote;
        // int retry_count = 0;
        // Read message in chunks, write to file
        while (file_b_wrote < msg_size) {
            b_left_to_write = msg_size - file_b_wrote;
            // read/ write in blocks
            ssize_t b_to_WR = min(BLOCK_SIZE, b_left_to_write);
            // fprintf(stderr, "b_to_WR: %ld\n", b_to_WR);
            // read from socket
            // WIP
            // ssize_t b_read = read_all_from_socket(client_fd, file_buffer, b_to_WR);
            ssize_t b_read = 0;
            while (b_read < (ssize_t) b_to_WR) {
                ssize_t cur_read = read(client_fd, file_buffer + b_read, b_to_WR - b_read);
                if (cur_read == 0) { break; } // Done reading
                else if (cur_read > 0) { b_read += cur_read; } // increment bytes read
                else if (cur_read == -1 && errno == EINTR) { continue; }// interruption, retry
                else { 
                    // error, update status
                    // perror("PUT_dynamic: failed to read msg\n");
                    status = -1;
                    if (errno == EAGAIN) {
                        // perror("PUT_dynamic: EAGAIN, nothing to read\n");
                        status = 2;
                    }
                    break;
                }
            }
            if (status == 0 && b_read == 0) {
                // no bytes read, move on
                perror("PUT_dynamic: EOF\n");
                status = 0;
                break;
            }
            // if (b_read < 0) {
            //     fprintf(stderr, "b_read: %ld\n", b_read);
            //     // Error reading, means client has not closed yet
            //     perror("PUT_dynamic: failed to read msg\n");
            //     status = -1; // TODO CHECK IF THIS BREAKS ANYTHING, JUST ADDED
            //     if (errno == EAGAIN) {
            //         // try to rerun PUT
            //         perror("PUT_dynamic: EAGAIN, nothing to read\n");
            //         status = 2; // uncomment when can handle this status type, if not will break
            //         // continue;
            //         // break;
            //         // return status;
            //     }
            //     break;
            // } else if (b_read == 0) {
            //     // Client closed writing on its end, cannot expect more bytes
            //     // break check for sizes
            //     fprintf(stderr, "b_read: %ld\n", b_read);
            //     status = 0; // CHECK FOR BREAKING
            //     break;
            // }

            // write to file
            ssize_t b_wrote = fwrite(file_buffer, 1, b_read, file);
            if (b_wrote < b_read) { // changed to b_read
                perror("PUT_dynamic: failed to write to file\n");
                status = -1;
                break;
            }
            file_b_wrote += b_wrote;
            current_client_info->bytes_processed = file_b_wrote;
        }
        // close file
        fclose(file);
        if (status != 0) {
            // retry
            return status;
        } else {
            // success, status == 0
            current_client_info->stage = FINAL_CHECKS;
        }
    }

    if (current_client_info->stage == FINAL_CHECKS) {
        print_client_info(current_client_info->client_fd);
        // variable setup
        ssize_t file_b_wrote = current_client_info->bytes_processed;
        ssize_t msg_size = current_client_info->msg_size;
        char file_buffer[BLOCK_SIZE];

        // check file sizes
        // Check too much data
        ssize_t b_read = read_all_from_socket(client_fd, file_buffer, 1);
        // fprintf(stderr, "Try to read more bytes: %ld\n", b_read);
        if (b_read > 0) {
            perror("PUT_request: too much data\n");
            status = -1;
        }
        // fprintf(stderr, "bytes written: %ld\n", file_b_wrote);
        // Check too little data
        if (file_b_wrote < msg_size) {
            perror("PUT_request: too little data\n");
            fprintf(stderr, "file_b_wrote: %ld msg_size: %ld\n", file_b_wrote, msg_size);
            status = -1;
        }
        // MAY NEED TO FURTHER BREAK THIS DOWN, if send response fails for either case
        // send ERROR response if status -1
        if (status == -1) {
            perror("PUT_request: status -1\n");
            send_response(ERROR, BAD_FILE_SIZE, client_fd);
            return EXIT_FAILURE;
        }
        // Send OK response: additional check added to only send if all bytes written
        if (file_b_wrote == msg_size && send_response(OK, NO_MSG, client_fd) < 0) {
            return EXIT_FAILURE;
        }
        current_client_info->stage = DONE;
    }
    print_client_info(current_client_info->client_fd);
    return status;
}

int process_client_request_dynamic(int client_fd) {
    // fprintf(stderr, "process_client_request_dynamic RUNNING\n");
    // check dictionary
    int status = 0;
    client_info *current_client_info = NULL;
    if (dictionary_contains(client_dictionary, &client_fd)) {
        current_client_info = dictionary_get(client_dictionary, &client_fd);
    } else {
        // new entry
        // Setup dictionary info and add to the dictionary
        current_client_info = init_empty_client_info();
        current_client_info->client_fd = client_fd;
        dictionary_set(client_dictionary, &client_fd, (void *) current_client_info);
        current_client_info->stage = PARSE;
    }
    print_client_info(current_client_info->client_fd);
    if (current_client_info->stage == PARSE) {
        // Get header
        char request[HEADER_SIZE];
        if (read_line(client_fd, request, sizeof(request)) < 0) {
            status = -1;
            print_invalid_response();
            perror("process_client_reponse: could not read line.\n");
            return status;
        }
        // Parse VERB
        char *space_pos = strchr(request, ' ');
        // set client info correctly
        if (!space_pos && strcmp(request, "LIST") == 0) {
            // handle LIST
            current_client_info->verb_ = LIST;
        } else if (space_pos) {
            // Check GET, PUT or DEL
            // split request line in 2
            *space_pos = '\0';
            char *filename = space_pos + 1;
            char *verb = request;
            // check filename length
            if (strlen(filename) > MAX_FILENAME_LENGTH) {
                status = -1;
                print_invalid_response();
                perror("process_client_reponse: MAX_FILENAME_LENGTH exceeded.\n");
                send_response(ERROR, HTTPS_BAD_REQUEST, client_fd);
                delete_remove_dictionary_entry(client_fd);
                return status;
            }
            // Dictionary set filename
            current_client_info->filename_length = strlen(filename);
            strcpy(current_client_info->filename, filename);

            // dispatch
            if (strcmp(verb, "GET") == 0) {
                current_client_info->verb_ = GET;
            } else if (strcmp(verb, "PUT") == 0) {
                current_client_info->verb_ = PUT;
            } else if (strcmp(verb, "DELETE") == 0) {
                current_client_info->verb_ = DELETE;
            } else {
                status = -1;
                print_invalid_response();
                perror("process_client_reponse: unknown verb. Removed entry\n");
                send_response(ERROR, HTTPS_BAD_REQUEST, client_fd);
                delete_remove_dictionary_entry(client_fd);
                return EXIT_FAILURE;
            }
        } else {
            status = -1;
            print_invalid_response();
            perror("process_client_reponse: unknown request. Removed entry\n");
            send_response(ERROR, HTTPS_BAD_REQUEST, client_fd);
            delete_remove_dictionary_entry(client_fd);
            return status;
        }
        current_client_info->stage = START;
    }

    // Call appropriate function
    if (current_client_info->stage != PARSE) { // needs to go in
        print_client_info(current_client_info->client_fd);
        switch (current_client_info->verb_) {
            case PUT:
                status = PUT_request_dynamic(current_client_info->client_fd);
                break;
            case GET:
                status = GET_request_dynamic(current_client_info->client_fd);
                break;
            case LIST:
                status = LIST_request(current_client_info->client_fd);
                break;
            case DELETE:
                status = DELETE_request(current_client_info->client_fd, current_client_info->filename);
                break;
            default:
                perror("SHOULD NOT BE HERE IN DISPATCH ERROR\n");
                status = EXIT_FAILURE;
                break;
        }
    }
    // update status
    current_client_info->status = status;
    // Remove only if the request is finished/ has error
    if (current_client_info->stage == DONE || current_client_info->status != 2) {
        print_client_info(client_fd);
        // fprintf(stderr, "Clinet_fd: %d removed from dictionary\n", client_fd);
        delete_remove_dictionary_entry(client_fd);
    }
    return status;
}

int process_epoll_events_dynamic(struct sockaddr_storage clientaddr, socklen_t client_addr_size,
                         int n_fds, struct epoll_event event, struct epoll_event *events) {
    // fprintf(stderr, "process_epoll_events_dynamic RUNNING\n");
    int status = 0;
    // cycle and process each event
    // fprintf(stderr, "Processing Epoll\n");
    for (int i = 0; i < n_fds; ++i) {
        if (events[i].data.fd == serverSocket) {
            // fprintf(stderr, "ACCEPTING NEW CLIENT\n");
            // serverSocket event; accept
            accept_new_client(clientaddr, client_addr_size, event);
            // fprintf(stderr, "FINISHED ACCEPTING NEW CLIENT\n");
        } else {
            // set up dictionary with details, resume when you get it again
            // client event, process and close
            int client_fd = events[i].data.fd;
            
            if (events[i].events & EPOLLIN) {
                // epoll as expected
                // fprintf(stderr, "PROCESSING EVENT %d CLIENT_FD: %d\n",i , client_fd);
                // first time
                status = process_client_request_dynamic(client_fd);

                // close client_fd and remove from monitoring
                if (status != 2) {
                    // fprintf(stderr, "Shutdown, Close, removed client_fd: %d\n", client_fd);
                    shutdown(client_fd, SHUT_RDWR);
                    close(client_fd);
                    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client_fd, NULL);
                } else {
                    // Change monitoring type to get more updates
                    event.events = EPOLLIN;         // monitor for input events (reading)
                    event.data.fd = client_fd;
                    epoll_ctl(epoll_fd, EPOLL_CTL_MOD, client_fd, &event);
                }
            } else if (events[i].events & EPOLLHUP || events[i].events & EPOLLERR) {
                // Epoll fd error
                perror("run_server: EPOLLHUP || EPOLERR.\n");
                close(client_fd);
                epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client_fd, NULL);
            }
        }
    }
    return status;
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
    memset(&event, 0, sizeof(event));
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

    // Setup dictionary to store info per client fd
    client_dictionary = int_to_shallow_dictionary_create();

    // main loop to proccess server stuff
    while(1) {
        // wait for events
        // fprintf(stderr, "epoll_wait\n");
        n_fds = epoll_wait(epoll_fd, events, EVENT_LIMIT, -1);
        // fprintf(stderr, "N_FDS EPOLL WAIT: %d\n", n_fds);
        // fprintf(stderr, "n_fds: %d\n", n_fds);
        if (n_fds < 0) {
            perror("run_server: epoll_wait error.\n");
            shutdown_server();
            exit(EXIT_FAILURE);
        }

        // Process events
        // process_epoll_events(clientaddr, client_addr_size, n_fds, event, events);
        process_epoll_events_dynamic(clientaddr, client_addr_size, n_fds, event, events);
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

// ----------------------------------------------------------------------------------------
// DEPRECATED FUNCTIONS
// ----------------------------------------------------------------------------------------
// // modularised put section RDWR to be dynamic
// // returns status
// int put_read_socket_write_file(client_info *current_client_info) { 
//     // setup of variables
//     ssize_t file_bytes_written = current_client_info->bytes_processed;
//     ssize_t msg_size = current_client_info->msg_size;
//     int client_fd = current_client_info->client_fd;
//     // fprintf(stderr, "msgsize: %ld\n", msg_size);
//     // open file
//     char *filename = current_client_info->filename;
//     // contruct path to file
//     char path[strlen(dir) + strlen(filename) + 2];
//     snprintf(path, sizeof(path), "%s/%s", dir, filename);
//     // fprintf(stderr, "PUT: %s\n", filename);
//     // open file
//     FILE *file = fopen(path, "a"); // make sure that we dont delete the file
//     if (file == NULL) {
//         perror("PRSWF: failed to open file\n");
//         send_response(ERROR, HTTPS_NOT_FOUND, client_fd);
//         return EXIT_FAILURE;
//     }
//     // Set the starting write position using fseek
//     if (fseek(file, file_bytes_written, SEEK_SET) != 0) {
//         perror("PRSWF: Failed to seek to required file position");
//         fclose(file);
//         send_response(ERROR, HTTPS_NOT_FOUND, client_fd);
//         return EXIT_FAILURE;
//     }
//     fprintf(stderr, "Starting off at byte %ld\n", file_bytes_written);
//     // Stuff i need to read/write the file with
//     int status = 0;
//     char file_buffer[BLOCK_SIZE];
//     ssize_t file_b_wrote = file_bytes_written;
//     ssize_t b_left_to_write = msg_size - file_b_wrote;
//     // int retry_count = 0;
//     // Read message in chunks, write to file
//     while (file_b_wrote < msg_size) {
//         b_left_to_write = msg_size - file_b_wrote;
//         // read/ write in blocks
//         ssize_t b_to_WR = min(BLOCK_SIZE, b_left_to_write);
//         // fprintf(stderr, "b_to_WR: %ld\n", b_to_WR);
//         // read from socket
//         ssize_t b_read = read_all_from_socket(client_fd, file_buffer, b_to_WR);
//         if (b_read < 0) {
//             fprintf(stderr, "b_read: %ld\n", b_read);
//             // Error reading, means client has not closed yet
//             perror("PRSWF: failed to read msg\n");
//             // status = -1;
//             if (errno == EAGAIN) {
//                 // try to rerun PUT
//                 perror("PRSWF: EAGAIN, nothing to read\n");
//                 status = 2; // uncomment when can handle this status type, if not will break
//                 // continue;
//                 // return status;
//             }
//             break;
//         } else if (b_read == 0) {
//             // Client closed writing on its end, cannot expect more bytes
//             // break check for sizes
//             fprintf(stderr, "b_read: %ld\n", b_read);
//             break;
//         }
//         // write to file
//         ssize_t b_wrote = fwrite(file_buffer, 1, b_read, file);
//         if (b_wrote < b_read) { // changed to b_read
//             perror("PRSWF: failed to write to file\n");
//             status = -1;
//             break;
//         }
//         file_b_wrote += b_wrote;
//         current_client_info->bytes_processed = file_b_wrote;
//     }
//     fclose(file);
//     // Handle no data in socket
//     if (status == 2) {
//         perror("PRSWF: 2 no data in socket, return later\n");
//         fprintf(stderr, "Left off at byte %ld\n", file_b_wrote);
//         return status;
//     }
//     // update client info
//     current_client_info->stage = DONE;

//     // check file sizes
//     // Check too much data
//     ssize_t b_read = read_all_from_socket(client_fd, file_buffer, 1);
//     fprintf(stderr, "Try to read more bytes: %ld\n", b_read);
//     if (b_read > 0) {
//         perror("PRSWF: too much data\n");
//         status = -1;
//     }
//     fprintf(stderr, "bytes written: %ld\n", file_b_wrote);
//     // Check too little data
//     if (file_b_wrote < msg_size) {
//         perror("PRSWF: too little data\n");
//         fprintf(stderr, "file_b_wrote: %ld msg_size: %ld\n", file_b_wrote, msg_size);
//         status = -1;
//     }
//     // send ERROR response if status -1
//     if (status == -1) {
//         perror("PRSWF: status -1\n");
//         send_response(ERROR, BAD_FILE_SIZE, client_fd);
//         return EXIT_FAILURE;
//     }
//     // Send OK response: additional check added to only send if all bytes written
//     if (file_b_wrote == msg_size && send_response(OK, NO_MSG, client_fd) < 0) {
//         return EXIT_FAILURE;
//     }
//     return status;
// }


// int GET_request(int client_fd, char *filename) {
//     // Grab dictionary entry, shallow copy is reference
//     client_info *current_client_info = dictionary_get(client_dictionary, &client_fd);
//     // Set Verb type and stage
//     current_client_info->verb_ = GET;
//     current_client_info->stage = OTHER;

//     // contruct path to file
//     char path[strlen(dir) + strlen(filename) + 2];
//     snprintf(path, sizeof(path), "%s/%s", dir, filename);
//     // get file stats, size
//     struct stat file_stat;
//     if (stat(path, &file_stat) != 0) {
//         perror("GET_request: failed to get file stat.\n");
//         send_response(ERROR, HTTPS_NOT_FOUND, client_fd);
//         return EXIT_FAILURE;
//     }
//     ssize_t file_size = file_stat.st_size;
//     // open file
//     FILE *file = fopen(path, "r");
//     if (file == NULL) {
//         perror("GET_request: failed to open file\n");
//         send_response(ERROR, HTTPS_NOT_FOUND, client_fd);
//         return EXIT_FAILURE;
//     }
//     // send response
//     // RESPONSE\n
//     // [Error Message]\n
//     // [File size][Binary Data]
//     if (send_response(OK, NO_MSG, client_fd) < 0) {
//         return EXIT_FAILURE;
//     }
//     if (send_message_size(client_fd, MESSAGE_SIZE_DIGITS, file_size) < 0) {
//         return EXIT_FAILURE;
//     }
//     // update client filesize
//     current_client_info->msg_size = file_size;
//     current_client_info->stage = RDWR_LOOP;

//     // Write file contents to socket
//     // Stuff i need to read/write the file with
//     int status = 0;
//     char file_buffer[BLOCK_SIZE];
//     ssize_t msg_b_wrote = 0;
//     ssize_t b_left_to_write = file_size;
//     // Read message in chunks, write to file
//     while (msg_b_wrote < file_size) {
//         b_left_to_write = file_size - msg_b_wrote;
//         // read/ write in blocks
//         ssize_t b_to_WR = min(BLOCK_SIZE, b_left_to_write);
//         // fprintf(stderr, "b_to_WR: %ld\n", b_to_WR);
//         // read from from file
//         ssize_t b_read = fread(file_buffer, 1, b_to_WR, file);
//         if (b_read < b_to_WR) {
//             perror("GET_request: failed to read from file\n");
//             status = -1;
//             break;
//         }
//         // write to socket
//         ssize_t b_wrote = write_all_to_socket(client_fd, file_buffer, b_read);
//         if (b_wrote < 0) {
//             perror("GET_request: failed to write msg\n");
//             status = -1;
//             break;
//         }
//         msg_b_wrote += b_wrote;
//         // update dictionary: use += b_wrote or = msg_b_wrote better?
//         current_client_info->bytes_processed = msg_b_wrote;
//     }
//     // update dictionary
//     current_client_info->stage = DONE;
//     // did not shutdown, done ltr
//     // close file
//     fclose(file);
//     return status;
// }

// int PUT_request(int client_fd, char *filename) {
//     // Grab dictionary entry, shallow copy is reference
//     client_info *current_client_info = dictionary_get(client_dictionary, &client_fd);
//     // Set Verb type and stage
//     current_client_info->verb_ = PUT;
//     current_client_info->stage = OTHER;

//     // contruct path to file
//     char path[strlen(dir) + strlen(filename) + 2];
//     snprintf(path, sizeof(path), "%s/%s", dir, filename);
//     fprintf(stderr, "PUT: %s\n", filename);
//     // open file
//     FILE *file = fopen(path, "w+");
//     if (file == NULL) {
//         perror("PUT_request: failed to open file\n");
//         send_response(ERROR, HTTPS_NOT_FOUND, client_fd);
//         return EXIT_FAILURE;
//     }
//     ssize_t msg_size = get_message_size(client_fd, MESSAGE_SIZE_DIGITS);
//     if (msg_size < 0) {
//         perror("PUT_request: failed get size\n");
//         send_response(ERROR, HTTPS_BAD_REQUEST, client_fd);
//         return EXIT_FAILURE;
//     }
//     // update client msgsize
//     current_client_info->msg_size = msg_size;
//     current_client_info->stage = RDWR_LOOP;

//     // // fprintf(stderr, "msgsize: %ld\n", msg_size);
//     // // Stuff i need to read/write the file with
//     int status = 0;
//     char file_buffer[BLOCK_SIZE];
//     ssize_t file_b_wrote = 0;
//     ssize_t b_left_to_write = msg_size;
//     // int retry_count = 0;
//     // Read message in chunks, write to file
//     while (file_b_wrote < msg_size) {
//         b_left_to_write = msg_size - file_b_wrote;
//         // read/ write in blocks
//         ssize_t b_to_WR = min(BLOCK_SIZE, b_left_to_write);
//         // fprintf(stderr, "b_to_WR: %ld\n", b_to_WR);
//         // read from socket
//         ssize_t b_read = read_all_from_socket(client_fd, file_buffer, b_to_WR);
//         if (b_read < 0) {
//             fprintf(stderr, "b_read: %ld\n", b_read);
//             // Error reading, means client has not closed yet
//             perror("PUT_request: failed to read msg\n");
//             // status = -1;
//             if (errno == EAGAIN) {
//                 // try to rerun PUT
//                 perror("PUT_request: EAGAIN, nothing to read\n");
//                 status = 2; // uncomment when can handle this status type, if not will break
//                 // continue;
//                 break;
//                 // return status;
//             }
//             break;
//         } else if (b_read == 0) {
//             // Client closed writing on its end, cannot expect more bytes
//             // break check for sizes
//             fprintf(stderr, "b_read: %ld\n", b_read);
//             break;
//         }
//         // write to file
//         ssize_t b_wrote = fwrite(file_buffer, 1, b_read, file);
//         if (b_wrote < b_read) { // changed to b_read
//             perror("PUT_request: failed to write to file\n");
//             status = -1;
//             break;
//         }
//         file_b_wrote += b_wrote;
//         current_client_info->bytes_processed = file_b_wrote;
//     }
//     // close file
//     fclose(file);

//     // Handle no data in socket
//     if (status == 2) {
//         perror("PUT_request: 2 no data in socket, return later\n");
//         fprintf(stderr, "Left off at byte %ld\n", file_b_wrote);
//         return status;
//     }
//     // update client info
//     current_client_info->stage = DONE;

//     // check file sizes
//     // Check too much data
//     ssize_t b_read = read_all_from_socket(client_fd, file_buffer, 1);
//     fprintf(stderr, "Try to read more bytes: %ld\n", b_read);
//     if (b_read > 0) {
//         perror("PUT_request: too much data\n");
//         status = -1;
//     }
//     fprintf(stderr, "bytes written: %ld\n", file_b_wrote);
//     // Check too little data
//     if (file_b_wrote < msg_size) {
//         perror("PUT_request: too little data\n");
//         fprintf(stderr, "file_b_wrote: %ld msg_size: %ld\n", file_b_wrote, msg_size);
//         status = -1;
//     }
//     // send ERROR response if status -1
//     if (status == -1) {
//         perror("PUT_request: status -1\n");
//         send_response(ERROR, BAD_FILE_SIZE, client_fd);
//         return EXIT_FAILURE;
//     }
//     // Send OK response: additional check added to only send if all bytes written
//     if (file_b_wrote == msg_size && send_response(OK, NO_MSG, client_fd) < 0) {
//         return EXIT_FAILURE;
//     }
//     return status;
// }

// int process_client_reponse(int client_fd) {
//     // Get header
//     int status = 0;
//     char request[HEADER_SIZE];
//     if (read_line(client_fd, request, sizeof(request)) < 0) {
//         status = -1;
//         print_invalid_response();
//         perror("process_client_reponse: could not read line.\n");
//         return status;
//     }
//     // Setup dictionary info and add to the dictionary
//     client_info *current_client_info = init_empty_client_info();
//     current_client_info->client_fd = client_fd;
//     dictionary_set(client_dictionary, &client_fd, (void *) current_client_info);

//     // Parse VERB
//     char *space_pos = strchr(request, ' ');
//     if (!space_pos && strcmp(request, "LIST") == 0) {
//         // handle LIST
//         status = LIST_request(client_fd);
//     } else if (space_pos) {
//         // Check GET, PUT or DEL
//         // split request line in 2
//         *space_pos = '\0';
//         char *filename = space_pos + 1;
//         char *verb = request;
//         // check filename length
//         if (strlen(filename) > MAX_FILENAME_LENGTH) {
//             status = -1;
//             print_invalid_response();
//             perror("process_client_reponse: MAX_FILENAME_LENGTH exceeded.\n");
//             send_response(ERROR, HTTPS_BAD_REQUEST, client_fd);
//             delete_remove_dictionary_entry(client_fd);
//             return status;
//         }
//         // Dictionary set filename
//         current_client_info->filename_length = strlen(filename);
//         strcpy(current_client_info->filename, filename);

//         // dispatch
//         if (strcmp(verb, "GET") == 0) {
//             status = GET_request(client_fd, filename);
//         } else if (strcmp(verb, "PUT") == 0) {
//             status = PUT_request(client_fd, filename);
//         } else if (strcmp(verb, "DELETE") == 0) {
//             status = DELETE_request(client_fd, filename);
//         } else {
//             status = -1;
//             print_invalid_response();
//             perror("process_client_reponse: unknown verb.\n");
//             send_response(ERROR, HTTPS_BAD_REQUEST, client_fd);
//             delete_remove_dictionary_entry(client_fd);
//             return EXIT_FAILURE;
//         }
//     } else {
//         status = -1;
//         print_invalid_response();
//         perror("process_client_reponse: unknown request.\n");
//         send_response(ERROR, HTTPS_BAD_REQUEST, client_fd);
//         delete_remove_dictionary_entry(client_fd);
//         return status;
//     }
//     // update status
//     current_client_info->status = status;
//     // Remove only if the request is finished/ has error
//     if (current_client_info->stage == DONE || current_client_info->status != 2) {
//         print_client_info(client_fd);
//         fprintf(stderr, "Clinet_fd: %d removed from dictionary\n", client_fd);
//         delete_remove_dictionary_entry(client_fd);
//     }
//     return status;
// }

// int process_epoll_events(struct sockaddr_storage clientaddr, socklen_t client_addr_size,
//                          int n_fds, struct epoll_event event, struct epoll_event *events) {
//     int status = 0;
//     // cycle and process each event
//     // fprintf(stderr, "Processing Epoll\n");
//     for (int i = 0; i < n_fds; ++i) {
//         if (events[i].data.fd == serverSocket) {
//             // fprintf(stderr, "ACCEPTING NEW CLIENT\n");
//             // serverSocket event; accept
//             accept_new_client(clientaddr, client_addr_size, event);
//             // fprintf(stderr, "FINISHED ACCEPTING NEW CLIENT\n");
//         } else {
//             // set up dictionary with details, resume when you get it again
//             // client event, process and close
//             int client_fd = events[i].data.fd;
            
//             if (events[i].events & EPOLLIN) {
//                 // epoll as expected
//                 // fprintf(stderr, "PROCESSING EVENT %d CLIENT_FD: %d\n",i , client_fd);
//                 // ADDED WIP for halfway done stuff
//                 if (dictionary_contains(client_dictionary, &client_fd)) {
//                     // fprintf(stderr, "Client is halfway %d\n", client_fd);
//                     client_info *current_client_info = dictionary_get(client_dictionary, &client_fd);
//                     // sanity checking
//                     if (current_client_info->stage == RDWR_LOOP && current_client_info->status == 2
//                         && current_client_info->verb_ == PUT) {
//                             status = put_read_socket_write_file(current_client_info);
//                             current_client_info->status = status;
//                             // Remove only if the request is finished/ has error
//                             if (current_client_info->stage == DONE || current_client_info->status != 2) {
//                                 print_client_info(client_fd);
//                                 // fprintf(stderr, "Clinet_fd: %d removed from dictionary\n", client_fd);
//                                 delete_remove_dictionary_entry(client_fd);
//                             }
//                         } else {
//                             perror("YOU DONE FUCKED UP BOI\n");
//                             print_client_info(client_fd);
//                             // help me
//                         }
//                 } else {
//                     // first time
//                     status = process_client_reponse(client_fd);
//                 }
//                 // close client_fd and remove from monitoring
//                 if (status != 2) {
//                     // fprintf(stderr, "Shutdown, Close, removed client_fd: %d\n", client_fd);
//                     shutdown(client_fd, SHUT_RDWR);
//                     close(client_fd);
//                     epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client_fd, NULL);
//                 } else {
//                     // Change monitoring type to get more updates
//                     //  this will break too much data
//                     struct epoll_event client_event;
//                     memset(&client_event, 0, sizeof(client_event));
//                     client_event.events = EPOLLIN;         // monitor for input events (reading)
//                     client_event.data.fd = client_fd;
//                     epoll_ctl(epoll_fd, EPOLL_CTL_MOD, client_fd, &client_event);
//                 }
//             } else if (events[i].events & EPOLLHUP || events[i].events & EPOLLERR) {
//                 // Epoll fd error
//                 perror("run_server: EPOLLHUP || EPOLERR.\n");
//                 close(client_fd);
//                 epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client_fd, NULL);
//             }
//         }
//     }
//     return status;
// }