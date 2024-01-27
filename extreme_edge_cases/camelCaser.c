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
        // move start till it hits a non punctuation mark
        while (ispunct((unsigned char)* start)) start++;
        // increment sentence index
        i++;
    }
    // need to check after last punctuation
    if (start < input + strlen(input)) {
        // allocate mem for new string + 1 for NULL
        char* last_sentence = (char *) malloc(strlen(start) + 1);
        // check malloc worked
        if (last_sentence) {
            // copy string
            strcpy(last_sentence, start);
            i++;
        }
    }
    sentences[*numSentences] = NULL;
    return sentences;
}

// Counts number of sentences by identifying punctuation marks
int countSentences(const char *input) {
    int count = 0;
    assert(input);
    const char *current = input;
    // Flag to indicate end of sentence
    // int sentenceEnd = 0;
    // int non_empty = 0;
    int first_sen = 1;
    // Cycle through until the end of the string
    while(*current) {
        printf("char: %c\n", *current);
        if (ispunct((unsigned char)* current) && *current) {            // deref pointer and cast to unsigned char
            while(*current) {
                printf("char: %c\n", *current);
                if (!ispunct((unsigned char)* current) && !isspace((unsigned char)* current)) {
                    printf("char: %c  count++\n", *current);
                    first_sen = 0;
                    count++;
                    break;
                }
                current++;
            }
            // if (!sentenceEnd) {                             // check for if first punctuation after some text
            //     printf("char: %c, count++\n", *current);
            //     count++;
            //     sentenceEnd = 1;                            // Mark end of sentence
            // }
        } else if (!isspace((unsigned char)* current) && first_sen) {    // check for first non-space and non-punctuation
            first_sen = 0;
            printf("first sentence count++\n");
            count++;
            // printf("r\n");
            //sentenceEnd = 0;                                // reset flag
        }
        current++;                                          // next char
    }
    printf("char: %c\n", *current);

    // if (count == 0) {                                       // if non empty but no punctuation marks
    //     count += non_empty;
    // }
    // else if (!sentenceEnd) {
    //     printf("count++ extra \n");
    //     count++;
    // }
    
    // else {
    //     // cycle to end of string to check for remaining words
    //     while (*current) {
    //         printf("while running");
    //         // word found
    //         if (!ispunct((unsigned char)* current) && !isspace((unsigned char)* current)) {
    //             printf("char: %c, extra count++\n", *current);
    //             count++;
    //             break;
    //         }
    //         current++;
    //     }
    // }

    return count;
}