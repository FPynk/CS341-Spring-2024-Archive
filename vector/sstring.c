/**
 * vector
 * CS 341 - Spring 2024
 */
#include "sstring.h"
#include "vector.h"

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <assert.h>
#include <string.h>

struct sstring {
    // Anything you want
    vector *char_vec;
};

sstring *cstr_to_sstring(const char *input) {
    assert(input);
    // your code goes here
    sstring *new_sstring = malloc(sizeof(sstring));
    if (!new_sstring) return NULL;

    new_sstring->char_vec = vector_create(char_copy_constructor,
                                          char_destructor,
                                          char_default_constructor);
    size_t length = strlen(input);
    for (size_t i = 0; i < length; ++i) {
        vector_push_back(new_sstring->char_vec, (void *) &input[i]);
    }

    return new_sstring;
}

char *sstring_to_cstr(sstring *input) {
    assert(input);
    assert(input->char_vec);
    // your code goes here
    size_t length = vector_size(input->char_vec);
    char *cstr = malloc(length + 1);
    if (!cstr) {
        perror("Failed malloc sstring to cstring");
        return NULL;
    }

    for (size_t i = 0; i < length; i++) {
        char *ch = vector_get(input->char_vec, i);
        cstr[i] = *ch;
    }
    cstr[length] = '\0';

    return cstr;
}

int sstring_append(sstring *this, sstring *addition) {
    assert(this);
    assert(addition);
    assert(this->char_vec);
    assert(addition->char_vec);
    // your code goes here
    size_t add_len = vector_size(addition->char_vec);

    for (size_t i = 0; i < add_len; ++i) {
        vector_push_back(this->char_vec, vector_get(addition->char_vec, i));
    }

    return vector_size(this->char_vec);
}

vector *sstring_split(sstring *this, char delimiter) {
    // your code goes here
    assert(this);

    // Vec to hold all substrings
    vector *out = vector_create(string_copy_constructor, string_destructor, string_default_constructor);
    assert(out);

    // temp vec for each sub string
    vector *temp_vec = vector_create(string_copy_constructor, string_destructor, string_default_constructor);
    if (!temp_vec) {
        vector_destroy(out);
        perror("Failed to malloc sstring_split");
        return NULL;
    }

    // iterate through all chars
    for (size_t i = 0; i < vector_size(this->char_vec); ++i) {
        char curr = *((char *) vector_get(this->char_vec, i));
        // check if delimiter reached or end of vec
        if (curr == delimiter || i == vector_size(this->char_vec) - 1) {
            //printf("char: %c delim or end\n", curr);
            // last char and not delimiter
            if (curr != delimiter) {
                vector_push_back(temp_vec, (void *) &curr);
            }

            // convert vec to ctring
            size_t substring_len = vector_size(temp_vec);
            char *temp = malloc((substring_len + 1) * sizeof(char));
            for (size_t j = 0; j < substring_len; ++j) {
                temp[j] = *((char *) vector_get(temp_vec, j));
            }
            temp[substring_len] = '\0';
            //printf("string to be added %s\n", temp);
            vector_push_back(out, temp);
            //printf("string added %s\n", (char *) *vector_back(out));
            free(temp);
            vector_clear(temp_vec);
            // might have to handle consec delimiters?
        } else {
            //printf("char: %c pb\n", curr);
            vector_push_back(temp_vec, (void *) &curr);
        }
    }
    // add "" if last ele is delimiter
    if (*((char *) vector_get(this->char_vec, vector_size(this->char_vec) - 1)) == delimiter) {
        char *temp = malloc(sizeof(char));
        temp[0] = '\0';
        vector_push_back(out, temp);
        free(temp);
    }
    vector_destroy(temp_vec);
    return out;
}

int sstring_substitute(sstring *this, size_t offset, char *target,
                       char *substitution) {
    // your code goes here
    assert(this);
    assert(offset < vector_size(this->char_vec));
    assert(target);
    assert(substitution);

    size_t target_len = strlen(target);
    size_t sub_len = strlen(substitution);
    int found = -1;

    // iterate starting from offset
    for (size_t i = offset; i < vector_size(this->char_vec) - target_len + 1; ++i) {
        // check target string starts at i
        int match = 1;
        for (size_t j = 0; j < target_len; ++j) {
            char *curr = (char *) vector_get(this->char_vec, i + j);
            if (*curr != target[j]) {
                match = 0;
                break;
            }
        }

        // match found
        if (match) {
            found = 0;

            // erase target
            for (size_t j = 0; j < target_len; ++j) {
                // printf("erasing: %c\n", *((char *) vector_get(this->char_vec, i)));
                vector_erase(this->char_vec, i);
            }

            // insert sub chars
            for (size_t j = 0; j < sub_len; ++j) {
                vector_insert(this->char_vec, i + j, (void *) &substitution[j]);
            }
            break;
        }
    }

    return found;
}

char *sstring_slice(sstring *this, int start, int end) {
    // your code goes here
    assert(this);
    assert(start >= 0) ;
    assert(end >= start);
    assert(end <= (int) vector_size(this->char_vec));

    int len = end - start;
    char *out = malloc((len + 1) * sizeof(char));
    assert(out);
    for (int i = 0; i < len; ++i) {
        out[i] = *((char *) vector_get(this->char_vec, i + start));
    }
    out[len] = '\0';

    return out;
}

void sstring_destroy(sstring *this) {
    // your code goes here
    assert(this);
    assert(this->char_vec);
    vector_destroy(this->char_vec);
    free(this);
}
