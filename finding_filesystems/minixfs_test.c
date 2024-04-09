/**
 * finding_filesystems
 * CS 341 - Spring 2024
 */
#include "minixfs.h"
#include "minixfs_utils.h"
#include <assert.h>
#include <stdio.h>

int main(int argc, char *argv[]) {
    // Write tests here!
    file_system *fs = open_fs("test.fs");
    // test chmod
    char *path = "/goodies/hello.txt";
    int new_perms = 0444; // prefix w 0 for octal in C
    // int old_perms = 0664; 
    int result = minixfs_chmod(fs, path, new_perms);
    printf("chmod result: %d\n", result);
    close_fs(&fs);
}
