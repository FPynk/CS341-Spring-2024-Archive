/**
 * perilous_pointers
 * CS 341 - Spring 2024
 */
#include "part2-functions.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * (Edit this function to print out the "Illinois" lines in
 * part2-functions.c in order.)
 */
int main() {
    // your code here
    int value = 81;
    first_step(value);

    int value2 = 132;
    second_step(&value2);

    int **value3 = malloc(sizeof(int*));
    *value3 = malloc(sizeof(int));
    **value3 = 8942;
    double_step(value3);
    free(*value3);
    free(value3);

    int **value4 = malloc(sizeof(int*) * 6);
    value4[5] = malloc(sizeof(int));
    *(value4[5]) = 15;
    strange_step((char *)value4);
    free(value4[5]);
    free(value4);

    return 0;
}
