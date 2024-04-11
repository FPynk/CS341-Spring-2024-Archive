/**
 * deepfried_dd
 * CS 341 - Spring 2024
 */
#include "format.h"
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <string.h>

#define DEFAULT_BLOCK_SIZE 512
static int sig_pr_stats = 0;

// Signal handler function for SIGUSR1
// set sig_pr_stats to 1 and print stats
void signal_handler(int s) {
    sig_pr_stats = s == SIGUSR1 ? 1 : sig_pr_stats;
}

int main(int argc, char **argv) {
    // set signal handler for SIGUSR1
    signal(SIGUSR1, signal_handler);

    // init vars
    int opt = 0;            // parse cmd line
    FILE *in = stdin;       // input file pointer
    FILE *out = stdout;     // output file pointer
    size_t skip_in = 0;     // NUmber of blocks to skip from input
    size_t skip_out = 0;    // No of blocks to skip output
    size_t block_copy = 0;  // no of blocks to copy
    size_t block_size = DEFAULT_BLOCK_SIZE; // size of each block when copying

    // Parse cmd line
    while((opt = getopt(argc, argv, "i:o:b:c:p:k:")) != -1) {
        switch (opt)
        {
        // input file
        case 'i':
            in = fopen(optarg, "r");
            if (in == NULL) {
                print_invalid_input(optarg);
                exit(EXIT_FAILURE);
            }
            break;
        // output file
        case 'o':
            out = fopen(optarg, "w+");
            if (out == NULL) {
                print_invalid_output(optarg);
                exit(EXIT_FAILURE);
            }
            break;
        case 'b':
            block_size = atoll(optarg);
            break;
        case 'c':
            block_copy = atoll(optarg);
            break;
        case 'p':
            skip_in = atoll(optarg);
            break;
        case 'k':
            skip_out = atoll(optarg);
            break;
        case '?':
            exit(EXIT_FAILURE);
        }
    }

    // set file ptr positions using skip info
    // Check fseek errors for input and output files
    // if (in != stdin && fseek(in, skip_in * block_size, SEEK_SET) != 0) {
    //     perror("Error seeking in input file");
    //     fclose(in);
    //     if (out != stdout) fclose(out);
    //     exit(EXIT_FAILURE);
    // }
    // if (out != stdout && fseek(out, skip_out * block_size, SEEK_SET) != 0) { // Skip if out is stdout
    //     perror("Error seeking in output file");
    //     fclose(out);
    //     if (in != stdin) fclose(in);
    //     exit(EXIT_FAILURE);
    // }
    fseek(in, skip_in * block_size, SEEK_SET);
    fseek(out, skip_out * block_size, SEEK_SET);
    // time op
    clock_t start = clock();

    // vars to track copy progress
    size_t f_blocks = 0; // full blocks in 
    size_t p_blocks = 0; // partial blocks in
    size_t c_size = 0;   // copied size, bytes

    // Copy data
    while (1) {
        // check EOF or block copy limit
        if (feof(in) || (block_copy != 0 && p_blocks + f_blocks == block_copy)) {
            break;
        }

        // signal print stats
        if (sig_pr_stats) {
            clock_t diff = clock() - start;
            long double time_spent = diff / CLOCKS_PER_SEC;
            // print stats
            print_status_report(f_blocks, p_blocks, f_blocks, p_blocks, c_size, time_spent);
            sig_pr_stats = 0;
        }

        // Buffer to read
        char buf[block_size];
        size_t read = fread((void *) buf, 1, block_size, in);
        // check ran out of data to read
        if (read == 0) {
            break;
        }
        // write full/partial blocks, update appropriate counters
        fwrite((void *) buf, read, 1, out);
        c_size += read;
        f_blocks += read == block_size ? 1 : 0;
        p_blocks += read == block_size ? 0 : 1;
    }
    // calc total time
    clock_t diff = clock() - start;
    long double time_spent = diff / CLOCKS_PER_SEC;
    // print
    print_status_report(f_blocks, p_blocks, f_blocks, p_blocks, c_size, time_spent);

    // Close files
    if (in != stdin) fclose(in);
    if (out != stdout) fclose(out);
    return 0;
}