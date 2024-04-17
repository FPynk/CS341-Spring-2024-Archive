/**
 * mad_mad_access_patterns
 * CS 341 - Spring 2024
 */
#include "tree.h"
#include "utils.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
/*
  Look up a few nodes in the tree and print the info they contain.
  This version uses fseek() and fread() to access the data.

  ./lookup1 <data_file> <word> [<word> ...]
*/
#define BTN BinaryTreeNode
bool find_helper(FILE *in, size_t offset, char *word){
    // set file pos to specified offset to begin reading node
    fseek(in, offset, SEEK_SET);

    // node to check
    BTN node;
    // read node from file
    fread(&node, sizeof(BTN), 1, in);
    // file ptr now looking at word
    // get word length
    char c;
    int len = 0;
    while((c = fgetc(in)) != '\0' && c != EOF) {
        ++len;
    }

    // grab word
    char node_word[len + 1];
    node_word[len] = '\0';
    // reset file ptr to start of word
    fseek(in, offset + sizeof(BTN), SEEK_SET);
    fread(node_word, 1, len, in);

    // compare search word with current node
    int found = strcmp(word, node_word);
    // current node contains word
    if (found == 0) {
        printFound(word, node.count, node.price);
        return true;
    } else if (found < 0 && node.left_child > 0) {
        // left tree contains value due to word's value (not count)
        // recurse
        return find_helper(in, node.left_child, word);
    } else if (found > 0 && node.right_child > 0) {
        return find_helper(in, node.right_child, word);
    }
    // return false if leaf reached and not found
    return false;
}

int main(int argc, char **argv) {
    // parse args and check good input
    if (argc < 3) {
        // incorrect input, print usage
        printArgumentUsage();
        exit(1);
    }
    // input file
    char *in_file_name = argv[1];
    // no of word args
    size_t n_words = argc - 2;
    // open input to read
    FILE *in_file = fopen(in_file_name, "r");
    // check file success/ exists
    if (in_file == NULL) {
        openFail(in_file_name);
        exit(2);
    }
    // Check first 4 bytes
    char first_four[5];
    first_four[4] = '\0';
    fread(first_four, 1, 4, in_file);
    if (strcmp(first_four, BINTREE_HEADER_STRING) != 0) {
        formatFail(in_file_name);
        exit(2);
    }

    for (size_t i = 0; i < n_words; ++i) {
        // recurse and find
        bool found = find_helper(in_file, BINTREE_ROOT_NODE_OFFSET, argv[i+2]);
        // print not found if not found
        if (!found) {
            printNotFound(argv[i + 2]);
        }
    }

    if (in_file) {
        fclose(in_file);
    }
    return 0;
}
