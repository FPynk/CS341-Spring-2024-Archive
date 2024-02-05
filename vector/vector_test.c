/**
 * vector
 * CS 341 - Spring 2024
 */
#include "vector.h"
#include <stdio.h>
#include <string.h>

// void *int_copy_constructor(void *elem) {
//     if (!elem) return NULL;
//     int *new_int = malloc(sizeof(int));
//     *new_int = *(int *)elem;
//     return new_int;
// }

// void int_destructor(void *elem) {
//     free(elem);
// }

// void *int_default_constructor(void) {
//     int *new_int = malloc(sizeof(int));
//     *new_int = 0; // Default value
//     return new_int;
// }

int main(int argc, char *argv[]) {
    // Write your test cases here

    int TC1 = 0;
    int TC2 = 0;
    int TC3 = 1;
    int TC4 = 1;
    // basic testing
    if (TC1) {
        // Test inserting elements
        vector *int_vector = vector_create(int_copy_constructor, int_destructor, int_default_constructor);
        int a = 10, b = 20;
        vector_push_back(int_vector, &a);
        vector_push_back(int_vector, &b);

        // Test getting and printing elements
        int *elem_a = vector_get(int_vector, 0);
        int *elem_b = vector_get(int_vector, 1);
        if (elem_a) printf("Element at 0: %d\n", *elem_a);
        if (elem_b) printf("Element at 1: %d\n", *elem_b);

        // Test erasing an element and resizing
        vector_erase(int_vector, 0);
        vector_resize(int_vector, 10); // Example of resizing

        // Clear the vector and test its size
        vector_clear(int_vector);
        printf("Vector size after clear: %zu\n", vector_size(int_vector));

        // Destroy the vector
        vector_destroy(int_vector);
    }
    // testing 100 elements
    if (TC2) {
        vector *int_vector = vector_create(int_copy_constructor, int_destructor, int_default_constructor);
        // Test inserting 100 elements using vector_push_back
        for (int i = 0; i < 100; ++i) {
            vector_push_back(int_vector, &i);
        }

        // Verify the elements and vector size
        printf("After pushing back 100 elements, vector size: %zu. vector cap: %zu\n", vector_size(int_vector), vector_capacity(int_vector));
        for (size_t i = 0; i < vector_size(int_vector); ++i) {
            int *elem = vector_get(int_vector, i);
            if (elem) printf("Element at %zu: %d\n", i, *elem);
        }

        // Clear the vector for the next test
        vector_clear(int_vector);

        // Test inserting 100 elements using vector_insert at the beginning
        for (int i = 0; i < 100; ++i) {
            vector_insert(int_vector, 0, &i); // Inserting at position 0 to test inserts other than push_back
        }

        // Verify the elements and vector size after inserts
        printf("After inserting 100 elements at position 0, vector size: %zu. vector cap: %zu\n", vector_size(int_vector), vector_capacity(int_vector));
        for (size_t i = 0; i < vector_size(int_vector); ++i) {
            int *elem = vector_get(int_vector, i);
            if (elem) printf("Element at %zu: %d\n", i, *elem);
        }

        // Destroy the vector
        vector_destroy(int_vector);
    }

    if (TC3) {
        vector* my_vector = vector_create(string_copy_constructor, string_destructor, string_default_constructor);

        // Fill the vector with test strings
        for (int i = 0; i < 5; ++i) {
            char temp[20];
            sprintf(temp, "Test String %d", i);
            char* str = strdup(temp); // Use strdup to dynamically allocate memory for the string
            vector_push_back(my_vector, str);
            free(str); // Free the temporary string, assuming vector_push_back makes a copy
        }

        // Display the vector contents before erasure
        printf("Vector before erasures:\n");
        for (size_t i = 0; i < vector_size(my_vector); ++i) {
            printf("%s\n", (char*)vector_get(my_vector, i));
        }

        // Erase a few elements from different positions
        vector_erase(my_vector, 0); // Beginning
        vector_erase(my_vector, 2); // Middle (note: indices shift after erasure)
        vector_erase(my_vector, vector_size(my_vector) - 1); // End

        // Display the vector contents after erasures but before clear
        printf("\nVector after erasures and before clear:\n");
        for (size_t i = 0; i < vector_size(my_vector); ++i) {
            printf("%s\n", (char*)vector_get(my_vector, i));
        }

        // Clear the vector to remove and free remaining elements
        vector_clear(my_vector);

        // Destroy the vector itself
        vector_destroy(my_vector);

        printf("\nTest completed: vector_erase and vector_clear\n");
    }

    if (TC4) {
        vector* test_vec1 = vector_create(int_copy_constructor, int_destructor, int_default_constructor);
        // Fill the vector with integers
        for (int i = 0; i < 5; ++i) {
            vector_push_back(test_vec1, (void *)&i);
        }

        // Display the vector contents before erasure
        printf("Vector before erasures:\n");
        for (size_t i = 0; i < vector_size(test_vec1); ++i) {
            int* item = (int*)vector_get(test_vec1, i);
            printf("%d\n", *item);
        }

        // Erase a few elements from different positions
        vector_erase(test_vec1, 0); // Beginning
        vector_erase(test_vec1, 2); // Middle (indices shift after erasure)
        vector_erase(test_vec1, vector_size(test_vec1) - 1); // End

        // Display the vector contents after erasures but before clear
        printf("\nVector after erasures and before clear:\n");
        for (size_t i = 0; i < vector_size(test_vec1); ++i) {
            int* item = (int*)vector_get(test_vec1, i);
            printf("%d\n", *item);
        }

        // Clear the vector to remove and free remaining elements
        vector_clear(test_vec1);

        // Destroy the vector itself
        vector_destroy(test_vec1);

        printf("\nTest completed: vector_erase and vector_clear\n");
    }
    return 0;
}
