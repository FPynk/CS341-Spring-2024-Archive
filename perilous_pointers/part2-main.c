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

    char value4[10];
    for (int i = 0; i < 10; i++) {
        value4[i] = 1;
    }
    *(int *)(value4 + 5) = 15;
    strange_step(value4);

    char value5[4];
    value5[3] = (char) 0;
    empty_step((void *) value5);

    char value6[4];
    value6[3] = 'u';
    two_step((void *) value6, value6);

    char value7[10];
    three_step(value7, &value7[2], &value7[4]);

    char value8[20];
    value8[0] = (char) 0;
    for (int i = 1; i < 4; i++) {
        value8[i] = value8[i-1] + 8;
    }
    step_step_step(value8, value8, value8);

    char value9[1];
    value9[0] = (char) 3;
    it_may_be_odd(value9, 3);

    char value10[] = "CS341,CS341";
    tok_step(value10);

    int value11 = 0x10001001;
    the_end(&value11, &value11);

    return 0;
}
