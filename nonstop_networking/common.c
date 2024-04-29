/**
 * nonstop_networking
 * CS 341 - Spring 2024
 */
#include "common.h"
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/socket.h>

// Reads count number of bytes from socket into buffer
// Returns 0 if no data read
// Returns -1 if error
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

// Writes count number of bytes from socket into buffer
// Returns 0 if no data read
// Returns -1 if error
ssize_t write_all_to_socket(int socket, const char *buffer, size_t count) {
    // Your Code Here
    ssize_t b_write = 0;
    while (b_write < (ssize_t) count) {
        ssize_t cur_write = write(socket, buffer + b_write, count - b_write);
        if (cur_write == 0) { break; } // done
        else if (cur_write > 0) { b_write += cur_write; } // increment
        else if (cur_write == -1 && errno == EINTR) { continue; } //interrupt, retry
        else { return -1; } // error
    }
    return b_write;
}

// returns size of message as ssize_t, converts from network to host byte order (reprecated)
// Returns 0 if no data read
// Returns -1 if error from read_all_from_socket
ssize_t get_message_size(int socket, size_t MESSAGE_SIZE_DIGITS) {
    int32_t size;
    ssize_t read_bytes =
        read_all_from_socket(socket, (char *)&size, MESSAGE_SIZE_DIGITS);
    fprintf(stderr, "Get_msg_size: bytes read %ld\n", read_bytes);
    if (read_bytes == 0 || read_bytes == -1)
        return read_bytes;
    return (ssize_t) size;
    // return (ssize_t)ntohl(size); // do not do this
}

// returns bytes written from write all bytes
ssize_t send_message_size(int socket, size_t MESSAGE_SIZE_DIGITS, size_t size) {
    ssize_t write_bytes =
        write_all_to_socket(socket, (char *)&size, MESSAGE_SIZE_DIGITS);
    return write_bytes;
}

// Returns min of the 2 vars
size_t min(size_t a, size_t b) {
    return a < b ? a : b;
}