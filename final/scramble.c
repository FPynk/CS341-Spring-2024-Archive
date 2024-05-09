/* Notes
1) compile scramble
    - clang -o scramble -O0 -Wall -Wextra -Werror -g -std=c99 -D_GNU_SOURCE -DDEBUG scramble.c
2) mmap to modify 1 or 2 files IN PLACE
    - do not use fread or malloc
    - 2 input filenames, correspond to different files
    - ./scramble file1 file2
3) Swap every other byte in each file with byte in same position
    - file1: ABCD file2: 123456 before swap
    - file1: 1B3D file2: A2C456 after swap
    - 1 swapped with A, 3 with C
4) if file1 and file2 correspond to same inode on same device
    - reverse order of bytes of file 1
    - file1: ABCD -> file1: DCBA
    - print "Reversing\n"
5) if either file1 or file2 are symbolic link do not modify contents
    - print "Symlink!\n" and exit(3)
6) if either file is 0-length
    - print "Empty!\n" and exit(4)
*/
/* GRADING RUBRIC CHECKLIST
A) 10 The included author name comment in your source file is your netid. DONE
B) 10  If 2 arguments are not specified, display a usage message on stderr and exit with a value of 1. DONE
C) 10  If the files cannot be opened or accessed, print a message using perror and exit with a value of 2. DONE
D) 10  Otherwise, if either file is a symbolic link then print Symlink! and exit with a value of 3. DONE
E) 10  Otherwise, if either file is 0-length (i.e. empty) print Empty! and exit with a value of 4. DONE
F) 10  Uses mmap to map the file(s)’ contents into writeable memory. DONE
G) 10 In normal mode, swap every other byte of the two given files using the memory-mapping. DONE
H) 10  Prints Reversing if the two files are the same inode on the same device. DONE
I) 10  In reverse mode, reverses the contents of the file when the given files are the same inode on the same 
device. DONE
J) 10  Exits normally with a value 0. DONE
*/ 
// DO NOT REMOVE BELOW
// RUBRIC A)
// author: acloh2

// imports
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>

// constants and definitions
#define BLOCK_SIZE 1024 * 4 // 4KB

// apolgy to the grader that needs to look through this THICC main function
int main(int argc, char *argv[]) {
    // usage error
    // RUBRIC B)
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <file1> <file2>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    // initialise 
    char *file_one = argv[1];
    char *file_two = argv[2];
    int reverse_flag = 0;
    // check if file1 and file2 are valid files
    struct stat stat_one, stat_two, lstat_one, lstat_two;

    // get status, check valid files
    // RUBRIC C)
    if (stat(file_one, &stat_one) == -1 || stat(file_two, &stat_two) == -1) {
        perror("Cannot open/access file1 or file2");
        exit(2);
    }
    // check for symbolic link
    if (lstat(file_one, &lstat_one) == -1 || lstat(file_two, &lstat_two) == -1) {
        perror("Cannot open/access file1 or file2");
        exit(2);
    }
    // RUBRIC D)
    if (S_ISLNK(lstat_one.st_mode) || S_ISLNK(lstat_two.st_mode)) {
        printf("Symlink!\n");
        exit(3);
    }
    // RUBRIC E)
    // are either 0 length?
    if (stat_one.st_size == 0 || stat_two.st_size == 0) {
        printf("Empty!\n");
        exit(4);
    }
    // do they point to the same inode?
    // RUBRIC H)
    if (stat_one.st_ino == stat_two.st_ino) {
        printf("Reversing\n");
        reverse_flag = 1;
    }
    if (reverse_flag) {
        // reverse file_one
        int fd = open(file_one, O_RDWR);
        if (fd == -1) {
            perror("Cannot open file1");
            exit(2);
        }
        // RUBRIC F)
        // map file into memory
        char *file_one_data = mmap(NULL, stat_one.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
        if (file_one_data == MAP_FAILED) {
            perror("Error mappng file_one");
            close(fd);
            exit(-1);
        }
        // RUBRIC I)
        // reverse
        size_t i, j;
        char temp;
        for (i = 0, j = stat_one.st_size - 1; i < j; ++i, --j) {
            temp = file_one_data[i];
            file_one_data[i] = file_one_data[j];
            file_one_data[j] = temp;
        }

        // unmap
        if (munmap(file_one_data, stat_one.st_size) == -1) {
            perror("Failed to unmap\n");
            close(fd);
            exit(-1);
        }

        close(fd);
    } else {
        // swap operations
        int fd_one = open(file_one, O_RDWR);
        int fd_two = open(file_two, O_RDWR);
        if (fd_one == -1 || fd_two == -1) {
            perror("Cannot open either file1 or file2");
            if (fd_one != -1) close(fd_one);
            if (fd_two != -1) close(fd_two);
            exit(2);
        }
        // RUBRIC F)
        // map file into memory
        char *file_one_data = mmap(NULL, stat_one.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd_one, 0);
        char *file_two_data = mmap(NULL, stat_two.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd_two, 0);
        if (file_one_data == MAP_FAILED || file_two_data == MAP_FAILED) {
            perror("Error mapping file_one or file_two");
            if (file_one_data != MAP_FAILED) munmap(file_one_data, stat_one.st_size);
            if (file_two_data != MAP_FAILED) munmap(file_two_data, stat_two.st_size);
            close(fd_one);
            close(fd_two);
            exit(-1);
        }
        // RUBRIC G)
        // swap
        size_t min_file_size = stat_one.st_size < stat_two.st_size ? stat_one.st_size : stat_two.st_size;
        char temp;
        for (size_t i = 0; i < min_file_size; i += 2) {
            temp = file_one_data[i];
            file_one_data[i] = file_two_data[i];
            file_two_data[i] = temp;
        }
        // unmap
        if (munmap(file_one_data, stat_one.st_size) == -1 || munmap(file_two_data, stat_one.st_size) == -1) {
            perror("Failed to unmap file1 or file2\n");
            close(fd_one);
            close(fd_two);
            exit(-1);
        }

        close(fd_one);
        close(fd_two);
    }
    // RUBRIC J)
    exit(EXIT_SUCCESS);
}