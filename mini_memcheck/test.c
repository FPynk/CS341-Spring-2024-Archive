/**
 * mini_memcheck
 * CS 341 - Spring 2024
 */
#include <stdio.h>
#include <stdlib.h>

#include "mini_memcheck.h"
#include "mini_memcheck.c"

void bad_free_test() {
    printf("Running bad_free_test...\n");
    mini_free((void*)0x12345678);
    void* ptr = mini_realloc((void*)0x87654321, 100,  "testfile", __builtin_return_address(0));
    mini_free(ptr); // Attempt to free the realloc'ed invalid pointer, should do nothing
    void* valid_ptr = mini_malloc(10, "testfile", __builtin_return_address(0));
    mini_free(valid_ptr);
    mini_free(valid_ptr); // Invalid double free
}

void calloc_test() {
    printf("Running calloc_test...\n");
    void* ptr1 = mini_calloc(5, sizeof(int), "testfile", __builtin_return_address(0));
    void* ptr2 = mini_calloc(10, sizeof(char), "testfile", __builtin_return_address(0));
    mini_free(ptr1);
    mini_free(ptr2);
}

void full_test() {
    printf("Running full_test...\n");
    void* ptr1 = mini_malloc(100, "testfile", __builtin_return_address(0));
    mini_free(ptr1);
    void* ptr2 = mini_malloc(200, "testfile", __builtin_return_address(0)); // Not freed -> memory leak
    void* ptr3 = mini_realloc(NULL, 50, "testfile", __builtin_return_address(0));
    mini_free(ptr3);
    mini_free((void*)0xABCDEF); // Invalid free
}

void leak_test() {
    printf("Running leak_test...\n");
    void* ptr1 = mini_malloc(50, "testfile", __builtin_return_address(0));
    void* ptr2 = mini_malloc(100, "testfile", __builtin_return_address(0)); // Not freed -> memory leak
}

void no_leak_test() {
    printf("Running no_leak_test...\n");
    void* ptr1 = mini_malloc(100, "testfile", __builtin_return_address(0));
    mini_free(ptr1);
    void* ptr2 = mini_calloc(1, 100, "testfile", __builtin_return_address(0));
    mini_free(ptr2);
    void* ptr3 = mini_malloc(50, "testfile", __builtin_return_address(0));
    void* ptr4 = mini_realloc(ptr3, 100, "testfile", __builtin_return_address(0));
    mini_free(ptr4);
}

void realloc_test() {
    printf("Running realloc_test...\n");
    void* ptr = mini_malloc(50, "testfile", __builtin_return_address(0));
    ptr = mini_realloc(ptr, 100, "testfile", __builtin_return_address(0));
    ptr = mini_realloc(ptr, 25, "testfile", __builtin_return_address(0));
    mini_free(ptr);
}

void realloc_test2() {
    //printf("Running realloc_test...\n");
    // void* ptr = mini_malloc(40, "testfile", __builtin_return_address(0));
    // ptr = mini_realloc(ptr, 60, "testfile", __builtin_return_address(0));
    // mini_free(ptr);
    void* ptr1 = malloc(40);
    ptr1 = realloc(ptr1, 60);
    void* ptr2 = realloc(ptr1, 60);
    free(ptr1);
}

void realloc_test3() {
    //printf("Running realloc_test...\n");
    // void* ptr = mini_malloc(40, "testfile", __builtin_return_address(0));
    // ptr = mini_realloc(ptr, 60, "testfile", __builtin_return_address(0));
    // mini_free(ptr);
    // void* ptr1 = malloc(0);
    void *ptr1 = realloc(NULL, 60);
    //void* ptr2 = realloc(ptr1, 60);
    free(ptr1);
}


int main(int argc, char *argv[]) {
    setvbuf(stdout, NULL, _IONBF, 0);
    if (0) {
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
        printf("Tests completed. PT1\n");
        fflush(stdout);
    }
    

    // More testing
    // bad_free_test();
    // calloc_test();
    // full_test(); // will have 1 leak
    // leak_test(); // will have 2 leaks
    // no_leak_test();
    // realloc_test();
    // printf("Tests completed. PT2. Expect 2 leaks\n");
    // fflush(stdout);

    // realloc testing
    // realloc_test2();
    realloc_test3();
    return 0;
}