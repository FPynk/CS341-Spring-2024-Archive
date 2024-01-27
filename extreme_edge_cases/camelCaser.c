/**
 * extreme_edge_cases
 * CS 341 - Spring 2024
 */
#include "camelCaser.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

char **camel_caser(const char *input_str) {
    // TODO: Implement me!
    return NULL;
}

void destroy(char **result) {
    // TODO: Implement me!
    return;
}

// HELPER FUNCTIONS BELOW
// Finds punctuation marks and splits input string into sentences
// pass by reference numSentences
char **splitSentences(const char *input, int *numSentences) {
    if (!input || !numSentences) return NULL;
    *numSentences = countSentences(input);
    if (*numSentences == 0) return NULL;

    // Allocate space for array of sentence pointers
    char **sentences = (char**) malloc((*numSentences + 1) * sizeof (char *));
    if (!sentences) return NULL;    // check malloc worked

    const char *start = input;      // Start of input
    const char *end = NULL;         // end
    int i = 0;                      // sentence index

    // iterate till no more found
    while ((end = strpbrk(start, "!\"#$%&'()*+,-./:;?@[\\]^_`{|}~")) != NULL) {
        // calc length
        int len = end - start;
        // allocate memory for sentence and add space for '\0'
        sentences[i] = (char *) malloc((len + 1)* sizeof(char));
        // check memory allocation worked
        if (!sentences[i]) {
            while (i > 0) free(sentences[--i]);
            free(sentences);
            return NULL;
        }
        // Copy sentence to array
        strncpy(sentences[i], start, len);
        // null terminate
        sentences[i][len] = '\0';
        // move start to one past the punctuation
        start = end + 1;
        // increment sentence index
        i++;
    }
    sentences[*numSentences] = NULL;
    return sentences;
}

// Counts number of sentences by identifying punctuation marks
int countSentences(const char *input) {
    int count = 0;
    assert(input);
    const char *current = input;

    while (*current) {
        if (ispunct((unsigned char)* current)) {
            count++;
        }
        current++;
    }
    return count;
}

void printCharArray(char **array) {
    if (!array) {
        printf("NULL\n"); // Handle NULL array
        printf("--------\n");
        return;
    }
    int i = 0;
    do {
        if (array[i] == NULL) {
            printf("NULL\n");
        } else {
            printf("%s\n", array[i]);
        }
        i++;
    } while (array[i] != NULL);
    if (array[i] == NULL) {
        printf("NULL\n");
    }
    printf("--------\n");
}