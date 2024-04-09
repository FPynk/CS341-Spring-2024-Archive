/**
 * finding_filesystems
 * CS 341 - Spring 2024
 */
#include "minixfs.h"
#include "minixfs_utils.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
    // Write tests here!
    file_system *fs = open_fs("test.fs");
    // test chmod chown
    printf("CHOWN/ CHMOD Test\n");
    struct stat statbuf;
    char *path = "/goodies/hello.txt";
    minixfs_stat(fs, path, &statbuf);
    printf("Mode bits before:\n");
    for (int i = 15; i >= 0; i -= 1){
        printf("%u ", (statbuf.st_mode >> i) & 1);
    }
    printf("\n");
    int new_perms = 0444; // prefix w 0 for octal in C
    int old_perms = 0664; 
    int result = minixfs_chmod(fs, path, new_perms);
    printf("chmod result: %d\n", result);
    minixfs_stat(fs, "/goodies/hello.txt", &statbuf);
    printf("Mode bits after:\n");
    for (int i = 15; i >= 0; i -= 1){
        printf("%u ", (statbuf.st_mode >> i) & 1);
    }
    printf("\n");
    minixfs_chmod(fs, path, old_perms);

    printf("Read test 1: Whole file, no off\n");
    char b0[16];
    off_t o0 = 0;
    ssize_t r0 = minixfs_read(fs, path, b0, 15, &o0);
    printf("Read %ld bytes:%s\n", r0, b0);

    printf("Read test 2: Offset to end of file, 2 off\n");
    char b1[16];
    off_t o1 = 2;
    ssize_t r1 = minixfs_read(fs, path, b1, 15, &o1);
    printf("Read %ld bytes:%s\n", r1, b1);

    printf("Read test 3: Offset to +x, 2 off\n");
    char b2[16];
    off_t o2 = 2;
    size_t c2 = 5;
    ssize_t r2 = minixfs_read(fs, path, b2, c2, &o2);
    printf("Read %ld bytes:%s\n", r2, b2);

    printf("Read test 4: Offset EOF, should not read\n");
    char b3[16];
    off_t o3 = 16;
    size_t c3 = 5;
    ssize_t r3 = minixfs_read(fs, path, b3, c3, &o3);
    printf("Read %ld bytes:%s\n", r3, b3);

    printf("\nWrite test 1: Write and extend file\n");
    struct stat statbuf1;
    char b4[32];
    char *w0 = "123456789ABCDEF";
    off_t o4 = 0;
    off_t ro4 = 0;
    size_t c4 = 16;
    ssize_t r4 = minixfs_read(fs, path, b4, 32, &ro4);
    ro4 = 0;
    minixfs_stat(fs, path, &statbuf1);
    printf("Before write, Read %ld bytes:%s", r4, b4);
    printf("filesize:%ld\n", statbuf1.st_size);
    r4 = minixfs_write(fs, path, w0, c4, &o4);
    minixfs_stat(fs, path, &statbuf1);
    printf("wrote %ld bytes\n", r4);
    r4 = minixfs_read(fs, path, b4, 32, &ro4);
    printf("After write, Read %ld bytes:%s\n", r4, b4);
    printf("filesize:%ld\n", statbuf1.st_size);

    printf("\nWrite test 2: Var Write tests\n");
    struct stat statbuf2;
    char b5[32];
    char *w1 = "XXXX";
    off_t o5 = 14;
    off_t ro5 = 0;
    size_t c5 = strlen(w1);
    ssize_t r5 = minixfs_read(fs, path, b5, 32, &ro5);
    ro5 = 0;
    minixfs_stat(fs, path, &statbuf2);
    printf("Before write, Read %ld bytes:%s\n", r5, b5);
    printf("filesize:%ld\n", statbuf2.st_size);
    r5 = minixfs_write(fs, path, w1, c5, &o5);
    minixfs_stat(fs, path, &statbuf2);
    printf("wrote %ld bytes\n", r5);
    r5 = minixfs_read(fs, path, b5, 32, &ro5);
    printf("After write, Read %ld bytes:%s\n", r5, b5);
    printf("filesize:%ld\n", statbuf2.st_size);

    close_fs(&fs);
}
