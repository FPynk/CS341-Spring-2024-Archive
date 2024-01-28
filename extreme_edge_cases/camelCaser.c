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
    if (input_str == NULL) return NULL;

    int numSentences = 0;
    // remember to free
    char **sentences = splitSentences(input_str, &numSentences);
    // Check for success
    if (sentences == NULL) {
        //printf("sentences: NULL'\n'");
        return NULL;
    }

    // allocate memory for array of pointers
    char** camelCasedArray = (char **) malloc((numSentences + 1) * sizeof(char *));
    // check success and free
    if (camelCasedArray == NULL) {
        //printf("camelCasedArray: NULL'\n'");
        destroy(sentences);
        return NULL;
    }
    // cycle through each sentence
    for (int i = 0; i < numSentences; i++) {
        int numWords = 0;
        // remember to free
        char **words = splitWords(sentences[i], & numWords);

        // estimate szie for new camelCased sentence per pointer
        int size = 1; // start with '\0'
        for (int j = 0; j < numWords; j++) {
            size += strlen(words[j]);
        }

        // allocate memory for that pointer to camelCased sentence
        camelCasedArray[i] = (char *) malloc(size * sizeof(char));
        // checking for success
        if (camelCasedArray[i] == NULL) {
            // free prev memory allocs
            while (i > 0) free(camelCasedArray[--i]);
            free(camelCasedArray);
            destroy(sentences);
            destroy(words);
        }

        camelCasedArray[i][0] = '\0';       // start with empty string
        // add on strings and camelCase as necessary
        for (int j = 0; j < numWords; j++) {
            toCamelCase(words[j], j == 0);
            strcat(camelCasedArray[i], words[j]);
        }

        destroy(words);
    }
    camelCasedArray[numSentences] = NULL; // NULL terminate
    destroy(sentences); // free mem

    return camelCasedArray;
}

void destroy(char **result) {
    // TODO: Implement me!
    if (result != NULL) {
        for (int i = 0; result[i] != NULL; i++) {
            free(result[i]);
        }
        free(result);
    }
    return;
}

// HELPER FUNCTIONS BELOW
// Finds punctuation marks and splits input string into sentences
// pass by reference numSentences
char **splitSentences(const char *input, int *numSentences) {
    if (!input || !numSentences) return NULL;
    *numSentences = countSentences(input);
    if (*numSentences == 0) {
        char **sentences = (char**) malloc((*numSentences + 1) * sizeof (char *));
        sentences[0] = NULL;
        return sentences;
    }

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

char **splitWords(const char *sentence, int *numWords) {
    // Check memory allocated
    if(!sentence || !numWords) return NULL;
    //printf("valid input\n");
    // copy original string
    char *sentenceCopy = strdup(sentence);
    // check success
    if (!sentenceCopy) return NULL;
    //printf("valid dup\n");   
    *numWords = countWords(sentence);
    //printf("numwords %d\n", *numWords);
    // allocate space for word array
    char **words = (char **) malloc((*numWords + 1) * sizeof(char *));
    // check for success
    if (!words) {
        free(sentenceCopy);
        return NULL;
    }
    //printf("valid array\n");
    // as per isspace()
    char *token = strtok(sentenceCopy, " \n\t\v\f\r");
    int i = 0;
    while(token != NULL) {
        //printf("While\n");
        //printf("token: |%s|\n", token);
        words[i++] = strdup(token);
        // NULL as strtok keeps track internally of where it is in the word
        token = strtok(NULL, " \n\t\v\f\r");
    }
    // NULL terminate array
    words[*numWords] = NULL;
    free(sentenceCopy);
    return words;
}

// counts number of words per sentence
int countWords(const char *sentence) {
    int count = 0;
    int inWord = 0;

    while (*sentence) {
        if (isspace((unsigned char) *sentence)) {
            inWord = 0;
        } else if (!inWord) {
            inWord = 1;
            count++;
        }
        sentence++;
    }
    return count;
}

// small word camel Caser with ability to select all lower or first upper and lower rest
void toCamelCase(char *word, int isFirstWord) {
    if (!word) return;

    if (isFirstWord) {
        toLowerCase(word);
    } else {
        toUpperCaseFirst(word);
    }
}

// converts word to all lower case
void toLowerCase(char *str) {
    if (!str) return;

    for (; *str; ++str) *str = tolower((unsigned char)* str);
}

// first letter UPPER and the rest lower, calls toLowerCase
void toUpperCaseFirst(char* str) {
    if (!str || !*str) return;
    *str = toupper((unsigned char)* str);
    toLowerCase(str + 1);
}

// Debugging print function
void printCharArray(char **array) {
    if (!array) {
        printf("NULL\n"); // Handle NULL array
        printf("Length: 0\n");
        printf("--------\n");
        return;
    }

    int i = 0;
    while (array[i] != NULL) {
        printf("%s\n", array[i]);
        i++;
    }
    printf("NULL\n");
    // printf("Length: %d\n", i);
    printf("--------\n");
}

void printPointerAddresses(char **array) {
    if (!array) {
        printf("Array is NULL\n");
        printf("--------\n");
        return;
    }

    int i = 0;
    while (array[i] != NULL) {
        printf("Pointer %d: %p\n", i, (void *)array[i]);
        i++;
    }
    printf("Pointer %d (End of array): %p\n", i, (void *)array[i]); // Print NULL terminator address

    printf("--------\n");
}