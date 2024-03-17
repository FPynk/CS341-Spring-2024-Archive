/**
 * password_cracker
 * CS 341 - Spring 2024
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <crypt.h>
#include <pthread.h>

#include "cracker1.h"
#include "format.h"
#include "utils.h"
#include "includes/queue.h"

#define SALT "xx"

typedef struct {
    char username[9]; // names 8 char + \0
    char password_hash[14]; // hashes 13 chars + \0
    char known_part[9]; // known part + unknown part 8 chars + \0
} task_details;

// Thread function handles memory freeing for queue
void *worker_thread_fn(void *arg) {
    queue *q = (queue *) arg;
    task_details *task;
    pthread_t id = pthread_self();
    struct crypt_data cdata;
    cdata.initialized = 0;
    //int q_cnt = 0;

    while(1) {
        //printf("before pull\n");
        task = (task_details *) queue_pull(q);
        //printf("pull cnt: %d\n", q_cnt++);
        // check if null task to terminate the thread
        if (strcmp(task->username, "XXXXXXXX") == 0) {
            free(task);
            break;
        }
        v1_print_thread_start(id, task->username);

        // prep for cracking
        char *base = task->known_part;
        // buffer to hold gen pws
        char test_pass[9];
        strncpy(test_pass, base, sizeof(test_pass));
        // grab len of prefix to do some math
        int prefix_len = getPrefixLength(base);
        int pw_len = strlen(base);
        // fill with a's to start
        memset(test_pass + prefix_len, 'a', pw_len - prefix_len);
        test_pass[pw_len] = '\0';
        bool found = 0;
        unsigned int hash_cnt = 0;
        do {
            // gen hash of pw
            char *hash = crypt_r(test_pass, SALT, &cdata);
            // check if pw found
            if (strcmp(hash, task->password_hash) == 0) {
                found = 1;
                printf("Password for %s is %s\n", task->username, test_pass);
                break;
            }
            hash_cnt++;
            if (hash_cnt % 1000 == 0) {
                printf("hash_cnt %u\n", hash_cnt);
            }
        } while (incrementString(test_pass + prefix_len)); // increment the unknown section

        if (!found) {
            printf("password not found\n");
        }

        free(task); // free task
    }
    return NULL;
}

int start(size_t thread_count) {
    printf("thread_count: %ld\n", thread_count);
    // TODO your code here, make sure to use thread_count!
    // Remember to ONLY crack passwords in other threads

    char line[256]; // hold line read from stdin
    // temp vars to hold parsed vals
    char username[9];
    char password_hash[14];
    char known_part[9];

    // queue set up
    queue *q = queue_create(10001); // fit null value
    // null task similar to sentinel value for strings since queue has no empty() function 
    task_details *null_task = malloc(sizeof(task_details));
    if (!null_task) exit(1);
    strncpy(null_task->username, "XXXXXXXX", 9);
    strncpy(null_task->password_hash, "XXXXXXXXXXXXX", 14);
    strncpy(null_task->known_part, "XXXXXXXX", 9);
    // DEBUG Count
    // unsigned int count = 0;

    // read & parse input lines
    while (fgets(line, sizeof(line), stdin)) {
        // use sscanf to parse the line
        if (sscanf(line, "%s %s %s", username, password_hash, known_part) == 3) {
            // copy into a new task details struct
            task_details *new_task = malloc(sizeof(task_details));
            if (new_task == NULL) exit(1);
            strncpy(new_task->username, username, sizeof(new_task->username));
            strncpy(new_task->password_hash, password_hash ,sizeof(new_task->password_hash));
            strncpy(new_task->known_part, known_part, sizeof(new_task->known_part));
            queue_push(q, new_task);
            //printf("%d\n", count++);
        } 
    }
    //printf("End of while\n");
    queue_push(q, null_task);
    //printf("Pushed null\n");
    // // DEBUG: Print out contents of queue to ensure all details added correctly
    // task_details *curr_task = (task_details *) queue_pull(q); // pull, cast and deref
    // while (strcmp(curr_task->username, null_task->username)) {
    //     printf("%s %s %s\n", curr_task->username, curr_task->password_hash, curr_task->known_part);
    //     free(curr_task);
    //     curr_task = (task_details *) queue_pull(q);
    // }

    // TODO: Create and manage threads to process each task
    pthread_t worker_thread;

    // 1 worker thread to test
    if (pthread_create(&worker_thread, NULL, worker_thread_fn, q) != 0) {
        perror("Failed to create worker thread");
        return 1;
    }
    // wait for worker thread to finish
    if (pthread_join(worker_thread, NULL) != 0) {
        perror("Failed ot join worker thread");
        return 1;
    }

    // memory management
    queue_destroy(q);

    return 0; // DO NOT change the return code since AG uses it to check if your
              // program exited normally
}
