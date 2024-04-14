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
        else if (cur_write == -1 && errno == EINTR) { continue; } //interrupt, retry
        else { return -1; } // error
    }
    return b_write;
}