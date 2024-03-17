/**
 * password_cracker
 * CS 341 - Spring 2024
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <crypt.h>
#include <pthread.h>
#include <math.h>

#include "cracker2.h"
#include "format.h"
#include "utils.h"
#include "includes/queue.h"

#define SALT "xx"
volatile bool password_found = false;
pthread_mutex_t mutex_global = PTHREAD_MUTEX_INITIALIZER;

typedef struct {
    char username[9]; // names 8 char + \0
    char password_hash[14]; // hashes 13 chars + \0
    char known_part[9]; // known part + unknown part 8 chars + \0
} task_details;

typedef struct {
    int thread_id;
    char username[9]; // names 8 char + \0
    char password_hash[14]; // hashes 13 chars + \0
    char known_part[9]; // known part + unknown part 8 chars + \0
    int unknown_len;
    volatile bool *password_found;
    pthread_mutex_t *mutex;
    int total_threads;
    long start_index;   // index to start at
    long count;         // no of PWs to try
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

    // ini local vars based on subrange and task details
    char current_attempt[9];
    setStringPosition(current_attempt, data->start_index);
    struct crypt_data cdata;
    cdata.initialized = 0;
    for (long i = 0; i < data->count && !*(data->password_found); ++i) {
        pthread_mutex_lock(data->mutex);
        bool found = *(data->password_found);
        pthread_mutex_unlock(data->mutex);

        if (found) break;
        // generate hash and test
        char *hash = crypt_r(current_attempt, SALT, &cdata);
        if (strcmp(hash, data->password_hash) == 0) {
            pthread_mutex_lock(data->mutex);
            *(data->password_found) = true;
            pthread_mutex_unlock(data->mutex);
        }

        incrementString(current_attempt);
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
    // push sentinel value
    push_sentinel_task(q);
    // TODO: Create and manage threads to process each task
    pthread_t *threads = malloc(thread_count * sizeof(pthread_t));
    thread_data *tdata = malloc(thread_count * sizeof(thread_data));

    if (!threads || !tdata) {
        perror("Failed alloc threads or tdata");
        return 1;
    }

    while (1)
    {
        // pull new task and check for sentinel value
        task_details *task = queue_pull(q);
        if (strcmp(task->username, "XXXXXXXX") == 0) {
            push_sentinel_task(q);
            free(task);
            break;
        }
        // reset per new task
        v2_print_start_user(task->username);
        password_found = false;

        for (size_t i = 0; i < thread_count; ++i) {
            // thread data set up
            tdata[i].thread_id = i + 1;
            tdata[i].password_found = &password_found;
            tdata[i].mutex = &mutex_global;
            tdata[i].total_threads = thread_count;
            // copy task details
            strcpy(tdata[i].username, task->username);
            strcpy(tdata[i].password_hash, task->password_hash);
            strcpy(tdata[i].known_part, task->known_part);

            // calc subrange
            int unknown_len = strlen(task->known_part) - getPrefixLength(task->known_part);
            tdata[i].unknown_len = unknown_len;
            // long total_combos = (long) pow(26, unknown_len);
            // set range of each thread
            getSubrange(unknown_len, thread_count, i + 1, &tdata[i].start_index, &tdata[i].count);

            if (pthread_create(&threads[i], NULL, worker_thread_fn, &tdata[i]) != 0) {
                perror("failed to create thread");
                return 1;
            }
        }

        for (size_t i = 0; i < thread_count; ++i) {
            pthread_join(threads[i], NULL);
        }
        // TODO: process results

        // mem management
        free(task);
    }
    
    // memory management
    queue_destroy(q);
    free(threads);
    free(tdata);
    return 0; // DO NOT change the return code since AG uses it to check if your
              // program exited normally
}
