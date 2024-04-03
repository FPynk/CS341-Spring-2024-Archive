/**
 * charming_chatroom
 * CS 341 - Spring 2024
 */
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/socket.h>

#include "utils.h"
static const size_t MESSAGE_SIZE_DIGITS = 4;

char *create_message(char *name, char *message) {
    int name_len = strlen(name);
    int msg_len = strlen(message);
    char *msg = calloc(1, msg_len + name_len + 4);
    sprintf(msg, "%s: %s", name, message);

    return msg;
}

ssize_t get_message_size(int socket) {
    int32_t size;
    ssize_t read_bytes =
        read_all_from_socket(socket, (char *)&size, MESSAGE_SIZE_DIGITS);
    if (read_bytes == 0 || read_bytes == -1)
        return read_bytes;

    return (ssize_t)ntohl(size);
}

// You may assume size won't be larger than a 4 byte integer
ssize_t write_message_size(size_t size, int socket) {
    // Your code here
    // convert size from host byte order to network byte order
    ssize_t fixed_size = htonl(size);
    // write size to sock
    return write_all_to_socket(socket, (char *) &fixed_size, MESSAGE_SIZE_DIGITS);
}

ssize_t read_all_from_socket(int socket, char *buffer, size_t count) {
    // Your Code Here
    ssize_t b_read = 0;
    while (b_read < (ssize_t) count) {
        ssize_t cur_read = read(socket, buffer + b_read, count - b_read);
        if (cur_read == 0) { break; } // Done reading
        else if (cur_read > 0) { b_read += cur_read; } // increment bytes read
        else if (cur_read == -1 && errno == EINTR) { continue; }// interruption, retry
        else { return -1; } // error
    }
    return b_read;
}

ssize_t write_all_to_socket(int socket, const char *buffer, size_t count) {
    // Your Code Here
    ssize_t b_write = 0;
    while (b_write < (ssize_t) count) {
        ssize_t cur_write = write(socket, buffer + b_write, count - b_write);
        if (cur_write == 0) { break; } // done
        else if (cur_write > 0) { b_write += cur_write; } // increment
        else if (cur_write == -1 & errno == EINTR) { continue; } //interrupt, retry
        else { return -1; } // error
    }
    return b_write;
}
