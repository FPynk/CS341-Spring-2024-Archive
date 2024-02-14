/**
 * mini_memcheck
 * CS 341 - Spring 2024
 */
#include <stdio.h>
#include <stdlib.h>

#include "mini_memcheck.h"
#include "mini_memcheck.c"

int main(int argc, char *argv[]) {
    // Test mini_malloc and mini_free
    // printf("Testing mini_malloc\n");
    // fflush(stdout);
    void *ptr1 = mini_malloc(100, "testfile", __builtin_return_address(0));
    if (ptr1 == NULL) {
        // printf("mini_malloc failed to allocate memory\n");
        // fflush(stdout);
    } else {
        // printf("Memory allocated with mini_malloc\n");
        // fflush(stdout);
    }
    // printf("Testing mini_free\n");
    // fflush(stdout);
    mini_free(ptr1);

    // Test mini_calloc
    // printf("Testing mini_calloc...\n");
    // fflush(stdout);
    void *ptr2 = mini_calloc(10, 10, "testfile2", __builtin_return_address(0));
    if (ptr2 == NULL) {
        // printf("mini_calloc failed to allocate memory\n");
        // fflush(stdout);
    } else {
        // printf("Memory allocated and zeroed with mini_calloc\n");
        // fflush(stdout);
    }
    // printf("Testing mini_free\n");
    // fflush(stdout);
    mini_free(ptr2);

    // Test mini_realloc - increasing size
    // printf("Testing mini_realloc for increasing size...\n");
    // fflush(stdout);
    void *ptr3 = mini_malloc(50, "testfile3", __builtin_return_address(0));
    void *ptr4 = mini_realloc(ptr3, 150, "testfile4", __builtin_return_address(0));
    if (ptr4 == NULL) {
        // printf("mini_realloc failed to reallocate memory\n");
        // fflush(stdout);
    } else {
        // printf("Memory reallocated with increased size using mini_realloc\n");
        // fflush(stdout);
    }
    // printf("Testing mini_free\n");
    // fflush(stdout);
    mini_free(ptr4);

    // Test mini_realloc - decreasing size
    // printf("Testing mini_realloc for decreasing size...\n");
    // fflush(stdout);
    void *ptr5 = mini_malloc(150, "testfile5", __builtin_return_address(0));
    void *ptr6 = mini_realloc(ptr5, 50, "testfile6", __builtin_return_address(0));
    if (ptr6 == NULL) {
        // printf("mini_realloc failed to reallocate memory\n");
        // fflush(stdout);
    } else {
        // printf("Memory reallocated with decreased size using mini_realloc\n");
        // fflush(stdout);
    }
    // printf("Testing mini_free\n");
    // fflush(stdout);
    mini_free(ptr6);

    // Test invalid free
    // printf("Testing invalid free...\n");
    // fflush(stdout);
    mini_free((void *)0x1234); // Attempting to free an invalid pointer

    // Finalize
    printf("Tests completed. Check the output for correctness.\n");
    fflush(stdout);
    return 0;
}