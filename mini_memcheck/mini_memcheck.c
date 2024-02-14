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
    if (num_elements == 0 || element_size == 0) {
        return NULL; // maybe assert false?
    }

    // calc total request size
    size_t total_size = num_elements * element_size;

    // alloc with mini malloc
    void *user_mem = mini_malloc(total_size, filename, instruction);
    if (!user_mem) {
        perror("mini_calloc faield to allocate memory");
        return NULL;
    }

    // zero out memory
    memset(user_mem, 0, total_size);
    return user_mem;
}

void *mini_realloc(void *payload, size_t request_size, const char *filename,
                   void *instruction) {
    // your code here
    // May need to update head
    // update total_memory_requested
    // Check invalid_addresses
    if (!payload) {
        // payload NULL, mini malloc
        return mini_malloc(request_size, filename, instruction);
    } else if (request_size == 0) {
        // request size is 0, mini free
        mini_free(payload);
        return NULL;
    }

    // Find corresponding metadata for payload
    meta_data *curr = head;
    meta_data *prev = NULL;
    // iterate and find
    while (curr != NULL) {
        if ((void *) (curr + 1) == payload) {
            break; // found
        }
        // updating pointers
        prev = curr;
        curr = curr->next;
    }

    if (!curr) {
        // payload not found
        invalid_addresses++;
        return NULL;
    }

    // Allocate new mem with increased size
    void *new_payload = mini_malloc(request_size, filename, instruction);
    if (!new_payload) {
        // failed to alloc
        return NULL;
    }

    // copy old data to new memory
    size_t old_size = curr->request_size;
    // 3rd arg: size given is the min of the 2, truncate if new size is smaller
    memcpy(new_payload, payload, old_size < request_size ? old_size : request_size);

    //free
    mini_free(payload);

    return new_payload;
}

void mini_free(void *payload) {
    // your code here
    // May need to update head
    // update total_memory_freed
    // invalid_addresses
    if (!payload) {
        // NULL, do nothing
        return;
    }

    // Check payload in list
    // pointer to pointer
    meta_data **indirect_ptr = &head;
    //iterate to end of list
    while (*indirect_ptr != NULL) {
        // point to metadata
        meta_data *curr = *indirect_ptr;
        // check curr payload matches payload to free
        if ((void *) (curr + 1) == payload) {
            // found, now remove
            // update pointer links/ relinking with next node
            *indirect_ptr = curr->next;
            // update total free
            total_memory_freed += curr->request_size;
            // free meta data and payload
            free(curr);
            // exit to avoid double free
            return;
        }
        indirect_ptr = &curr->next;
    }
    // not found
    invalid_addresses++;
}
