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

typedef struct {
    queue *q;
    int thread_id;
} thread_data;

void push_sentinel_task(queue *q) {
    // null task similar to sentinel value for strings since queue has no empty() function 
    task_details *null_task = malloc(sizeof(task_details));
    if (!null_task) exit(1);
    strncpy(null_task->username, "XXXXXXXX", 9);
    strncpy(null_task->password_hash, "XXXXXXXXXXXXX", 14);
    strncpy(null_task->known_part, "XXXXXXXX", 9);
    queue_push(q, null_task);
}

// Thread function handles memory freeing for queue
void *worker_thread_fn(void *arg) {
    thread_data *data = (thread_data *) arg;
    queue *q = data->q;
    task_details *task;
    pthread_t id = data->thread_id;
    struct crypt_data cdata;
    cdata.initialized = 0;
    //int q_cnt = 0;
    // need this to return info to main
    unsigned int *successes = malloc(sizeof(unsigned int));
    if (successes == NULL) {
        perror("Failed to alloc mem for succeses");
        return NULL;
    }
    *successes = 0;

    while(1) {
        //printf("before pull\n");
        task = (task_details *) queue_pull(q);
        //printf("pull cnt: %d\n", q_cnt++);
        // check if null task to terminate the thread
        if (strcmp(task->username, "XXXXXXXX") == 0) {
            push_sentinel_task(q);
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
        bool result = 1;
        unsigned int hash_cnt = 0;
        double start_thread_cpu_time = getThreadCPUTime();
        do {
            // gen hash of pw
            char *hash = crypt_r(test_pass, SALT, &cdata);
            // check if pw found
            if (strcmp(hash, task->password_hash) == 0) {
                result = 0;
                (*successes)++;
                // printf("Password for %s is %s\n", task->username, test_pass);
                break;
            }
            hash_cnt++;
            // if (hash_cnt % 1000 == 0) {
            //     printf("hash_cnt %u\n", hash_cnt);
            // }
        } while (incrementString(test_pass + prefix_len)); // increment the unknown section
        double end_thread_cpu_time = getThreadCPUTime();
        // if (result) {
        //     printf("password not found\n");
        // }
        v1_print_thread_result(id, task->username, test_pass, hash_cnt, end_thread_cpu_time - start_thread_cpu_time, result);
        free(task); // free task
    }
    return (void *) successes;
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
    // DEBUG Count
    unsigned int count = 0;

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
            count++;
            //printf("%d\n", count++);
        } 
    }
    //printf("End of while\n");
    // push sentinel value
    push_sentinel_task(q);
    //printf("Pushed null\n");
    // // DEBUG: Print out contents of queue to ensure all details added correctly
    // task_details *curr_task = (task_details *) queue_pull(q); // pull, cast and deref
    // while (strcmp(curr_task->username, null_task->username)) {
    //     printf("%s %s %s\n", curr_task->username, curr_task->password_hash, curr_task->known_part);
    //     free(curr_task);
    //     curr_task = (task_details *) queue_pull(q);
    // }

    // TODO: Create and manage threads to process each task
    pthread_t *threads = malloc(thread_count * sizeof(pthread_t));
    thread_data *tdata = malloc(thread_count * sizeof(thread_data));

    if (!threads) {
        perror("Failed to alloc threads");
        return 1;
    }
    unsigned int total_successes = 0;
    for (size_t i = 0; i < thread_count; ++i) {
        tdata[i].thread_id = i + 1;
        tdata[i].q = q;
        // 1 worker thread to test
        if (pthread_create(&threads[i], NULL, worker_thread_fn, &tdata[i]) != 0) {
            perror("Failed to create worker thread");
            return 1;
        }
        //printf("create for loop %ld\n", i);
    }
    //printf("all threads created\n");
    for (size_t i = 0; i < thread_count; ++i) {
        //printf("Join for loop begin %ld\n", i);
        // wait for worker thread to finish
        unsigned int *thread_successes;
        if (pthread_join(threads[i], (void **) &thread_successes) != 0) {
            perror("Failed to join worker thread");
            return 1;
        } else if (thread_successes != NULL) {
            // printf("good join\n");
            total_successes += *thread_successes;
            free(thread_successes);
        }
        //printf("Join for loop end %ld\n", i);
    }
    v1_print_summary(total_successes, count - total_successes);
    task_details *last_task = queue_pull(q);
    free(last_task);
    // memory management
    queue_destroy(q);
    free(threads);
    free(tdata);

    return 0; // DO NOT change the return code since AG uses it to check if your
              // program exited normally
}
