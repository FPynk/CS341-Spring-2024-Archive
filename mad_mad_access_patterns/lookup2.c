/**
 * mad_mad_access_patterns
 * CS 341 - Spring 2024
 */
#include "tree.h"
#include "utils.h"

#include <sys/stat.h>
#include <sys/mman.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
/*
  Look up a few nodes in the tree and print the info they contain.
  This version uses mmap to access the data.

  ./lookup2 <data_file> <word> [<word> ...]
*/
#define BTN BinaryTreeNode

int find_helper(BTN *node, char *word, char *addr) {
    if (strcmp(node->word, word) == 0) {
        printFound(node->word, node->count, node->price);
        return 1;
    }

    BTN *left_child = NULL;
    BTN *right_child = NULL;
    // calc pointers to children
    if (node->left_child > 0) {
        left_child = (BTN *) (addr + node->left_child);
    }
    if (node->right_child > 0) {
        right_child = (BTN *) (addr + node->right_child);
    }

    // Recurse and search
    int val = strcmp(word, node->word);
    if (left_child && val < 0) {
        return find_helper(left_child, word, addr);
    } else if (right_child && val > 0) {
        return find_helper(right_child, word, addr);
    }
    return 0;
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
    // open file to map into mem
    struct stat file_stat;
    int fd = open(in_file_name, O_RDONLY);
    if (fd == -1) {
        openFail(in_file_name);
        exit(2);
    }
    // get stat for file size
    fstat(fd, &file_stat);
    char *addr = mmap(NULL, file_stat.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (addr == MAP_FAILED) {
        mmapFail(in_file_name);
        exit(3);
    }

    // Check BTRE
    char first_four[4] = {'B', 'T', 'R', 'E'};
    if (memcmp(addr, first_four, 4) != 0) {
        formatFail(in_file_name);
        exit(2);
    }

    // get pointer to root node
    BTN *root = (BTN *) (addr + 4);
    // loop thru each word and search
    for (int i = 2; i < argc; ++i) {
        char *word = argv[i];
        int result = find_helper(root, word, addr);
        if (result == 0) {
            printNotFound(word);
        }
    }

    // unmap mem adn close file
    if (munmap(addr, file_stat.st_size) == -1) {
        mmapFail(in_file_name);
        exit(3);
    }
    if (fd) {
        close(fd);
    }
    return 0;
}
