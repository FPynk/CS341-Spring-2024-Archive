/**
 * nonstop_networking
 * CS 341 - Spring 2024
 */
#pragma once
#include <stddef.h>
#include <sys/types.h>

#define LOG(...)                      \
    do {                              \
        fprintf(stderr, __VA_ARGS__); \
        fprintf(stderr, "\n");        \
    } while (0);

typedef enum { GET, PUT, DELETE, LIST, V_UNKNOWN } verb;

ssize_t read_all_from_socket(int socket, char *buffer, size_t count);
ssize_t write_all_to_socket(int socket, const char *buffer, size_t count);
ssize_t get_message_size(int socket, size_t MESSAGE_SIZE_DIGITS);
ssize_t send_message_size(int socket, size_t MESSAGE_SIZE_DIGITS, size_t size);
size_t min(size_t a, size_t b);