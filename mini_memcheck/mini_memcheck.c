/**
 * mini_memcheck
 * CS 341 - Spring 2024
 */
#include "mini_memcheck.h"
#include <stdio.h>
#include <string.h>

void *mini_malloc(size_t request_size, const char *filename,
                  void *instruction) {
    // your code here
    // allocate memory for new metadata + data required
    meta_data *m_data = malloc(sizeof(meta_data) + request_size);
    // malloc status
    if (!m_data) {
        perror("mini_malloc m_data failed");
        return NULL;
    }
    // Update meta_data
    m_data->request_size = request_size;
    m_data->filename = filename;
    m_data->instruction = instruction;
    m_data->next = NULL;
    // update total memory requested
    total_memory_requested += request_size;
    // Case 1: First entry
    if (!head) {
        head = m_data;
        return (void *) head + 1;
    }
    // Must traverse list if not first meta_data entry
    meta_data *curr = head;
    while (curr->next) {
        curr = curr->next;
    }
    // append new meta data at end of the list
    curr->next = m_data;

    return (void *) (m_data + 1);
}

void *mini_calloc(size_t num_elements, size_t element_size,
                  const char *filename, void *instruction) {
    // your code here
    // update total_memory_requested
    return NULL;
}

void *mini_realloc(void *payload, size_t request_size, const char *filename,
                   void *instruction) {
    // your code here
    // May need to update head
    // update total_memory_requested
    // Check invalid_addresses
    return NULL;
}

void mini_free(void *payload) {
    // your code here
    // May need to update head
    // update total_memory_freed
    // invalid_addresses
}
