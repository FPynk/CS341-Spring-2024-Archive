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
pthread_mutex_t mutex_global = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

// TODO:
    // TASK:
    // set mutex lock
    // Set count for task
    // Set Count for completed
    // Store password in task_details
    // Store start_time of CPU
    // Store start_time of Wall
    // Have first thread initialise those 2 variables
    // Have last thread calc end time and print vars
    // Store result of password
    // Shift unknown len, total threads here
    // start index and count will be calced per thread and not stored
    // long start_index;   // index to start at
    // long count;         // no of PWs to try
typedef struct {
    pthread_mutex_t task_mutex;
    pthread_cond_t task_cond;
    char username[9]; // names 8 char + \0
    char password_hash[14]; // hashes 13 chars + \0
    char known_part[9]; // known part + unknown part 8 chars + \0
    int use_count; // when 0 it means you're the last person who should be touching this task
    // counts how many threads are done with the task, goes towards 0, 
    // use this in conj with local var to wait, will need to reset local var
    int complete_count;
    int delete_count;
    volatile char *password; // actual password if found
    volatile bool password_found;
    int unknown_len;
    int total_threads;
    double CPU_start;
    double wall_start;
    int hash_count;
} task_details;

typedef struct {
    queue *q;
    int thread_id;
} thread_data;

pthread_mutex_t pull_mutex = PTHREAD_MUTEX_INITIALIZER;
task_details *curr_task = NULL;

task_details *push_sentinel_task(queue *q, size_t thread_count) {
    // null task similar to sentinel value for strings since queue has no empty() function 
    task_details *null_task = malloc(sizeof(task_details));
    if (!null_task) exit(1);
    strncpy(null_task->username, "XXXXXXXX", 9);
    strncpy(null_task->password_hash, "XXXXXXXXXXXXX", 14);
    strncpy(null_task->known_part, "XXXXXXXX", 9);
    null_task->use_count = thread_count; // when 0 it means you're the last person who should be touching this task
    null_task->delete_count = thread_count;
    queue_push(q, null_task);
    return null_task;
}

void *worker_thread_fn(void *arg) {
    thread_data *data = (thread_data *) arg;
    queue *q = data->q;
    task_details *task;
    pthread_t id = data->thread_id;
    struct crypt_data cdata;
    cdata.initialized = 0;
    //int q_cnt = 0;
    // count hashes
    while(1) {
        //printf("before pull\n");
        pthread_mutex_lock(&pull_mutex);
        // not first
        if (curr_task != NULL) {
            task = curr_task;
            pthread_mutex_lock(&task->task_mutex);
            task->use_count--;
            pthread_mutex_unlock(&task->task_mutex);
        } else {
            task = (task_details *) queue_pull(q);
            curr_task = task;
            pthread_mutex_lock(&task->task_mutex);
            task->CPU_start = getCPUTime();
            task->wall_start = getTime();
            task->use_count--;
            pthread_mutex_unlock(&task->task_mutex);
        }
        pthread_mutex_unlock(&pull_mutex);
        // Check if NULL sentinel task, decrement use_count
        if (strcmp(task->username, "XXXXXXXX") == 0) {
            pthread_mutex_lock(&task->task_mutex);
            // Break and exit out of loop so it can join with main
            task->use_count--;
            pthread_mutex_unlock(&task->task_mutex);
            break;
        }
        
        //printf("pull cnt: %d\n", q_cnt++);
        // Check if first to grab task
        // if first: Update task start CPU and wall time
        // update use_count by minusing to count how many threads took task
        // If not first: update use_count
        

        // calculate subrange based on id and total threads
        long int start_index = 0;
        long int count = 0;
        pthread_mutex_lock(&task->task_mutex);
        getSubrange(task->unknown_len, task->total_threads, id, &start_index, &count);
        pthread_mutex_unlock(&task->task_mutex);

        // ini local vars based on subrange and task details
        char test_pass[9];
        strncpy(test_pass, task->known_part, sizeof(test_pass));
        // get pointer to unknown part and pass that to set string position
        char *unknown_part = test_pass + getPrefixLength(test_pass);
        setStringPosition(unknown_part, start_index);

        pthread_mutex_lock(&task->task_mutex);
        v2_print_thread_start(id, task->username, start_index, test_pass);
        pthread_mutex_unlock(&task->task_mutex);

        // result of crack
        int result = 2;
        int hash_count = 0;
        // Iterate and generate hashes for passwords
        for (long i = 0; i < count; ++i) { // might wanna shift this inside
            // IF password is found by self or others/ ran through all combos
            // Increment complete and wait for threads till all are complete and ready to move on
            // if password is found by self update password and password_found
            // someone else found it
            // generate hash and test
            pthread_mutex_lock(&task->task_mutex);
            if (task->password_found) {
                pthread_mutex_unlock(&task->task_mutex);
                break;
            }
            pthread_mutex_unlock(&task->task_mutex);

            char *hash = crypt_r(test_pass, SALT, &cdata);
            hash_count++;
            if (strcmp(hash, task->password_hash) == 0) {
                pthread_mutex_lock(&task->task_mutex);
                if (!task->password_found) { // check again to avoid race cond
                    task->password_found = true;
                    task->password= strdup(test_pass); // allocate mem and give to password
                    result = 0;
                } else {
                    // someone else found it
                    result = 1;
                }
                pthread_mutex_unlock(&task->task_mutex);
                break;
            }
            incrementString(test_pass);
        }
        if (hash_count < count && result != 0) {
            result = 1;
        }

        pthread_mutex_lock(&task->task_mutex);
        task->complete_count--;
        bool is_last = task->complete_count == 0;
        v2_print_thread_result(id, hash_count, result);
        task->hash_count += hash_count;
        // check if last to finish task (check complete counter)
        // if last: get end CPU and wall time and print
        if (is_last) {
            double CPU_end = getCPUTime();
            double wall_end = getTime();
            // print stuff
            v2_print_summary(task->username, 
                             (char *) task->password, 
                             task->hash_count, 
                             wall_end - task->wall_start, 
                             CPU_end - task->CPU_start, 
                             !task->password_found);
            // broadcast to wakeup
            curr_task = NULL;
            pthread_cond_broadcast(&task->task_cond);
            pthread_mutex_unlock(&task->task_mutex);
        } else {
            // wait
            while (task->complete_count > 0) {
                pthread_cond_wait(&task->task_cond, &task->task_mutex);
            }
            pthread_mutex_unlock(&task->task_mutex);
        }
        // need to free task memory
        pthread_mutex_lock(&pull_mutex);
        pthread_mutex_lock(&task->task_mutex);
        if (--task->delete_count == 0) {
            pthread_mutex_unlock(&task->task_mutex);
            free(task);
        } else {
            pthread_mutex_unlock(&task->task_mutex);
        }
        pthread_mutex_unlock(&pull_mutex);
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
            new_task->task_mutex = mutex_global;
            new_task->task_cond = cond;
            new_task->use_count = thread_count;
            new_task->complete_count = thread_count;
            new_task->delete_count = thread_count;
            new_task->password = "";
            new_task->password_found = 0;
            new_task->unknown_len = strlen(known_part) - getPrefixLength(known_part);
            new_task->total_threads = thread_count;
            new_task->CPU_start = 0;
            new_task->wall_start = 0;
            new_task->hash_count = 0;
            queue_push(q, new_task);
            count++;
            //printf("%d\n", count++);
        }
    }
    // push sentinel value
    task_details *null_task = push_sentinel_task(q, thread_count);
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
            free(task);
            break;
        }
        // reset per new task
        v2_print_start_user(task->username);
        password_found = false;
        volatile char *password = malloc(9 * sizeof(char));
        password[0] = '\0';
        double wall_start_time = getTime();
        double CPU_start_time = getCPUTime();
        for (size_t i = 0; i < thread_count; ++i) {
            // thread data set up
            tdata[i].thread_id = i + 1;
            tdata[i].password_found = &password_found;
            tdata[i].mutex = &mutex_global;
            tdata[i].total_threads = thread_count;
            tdata[i].password = &password;
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
        unsigned int total_hash = 0;
        for (size_t i = 0; i < thread_count; ++i) {
            unsigned int *hash_cnt;
            pthread_join(threads[i], (void **) &hash_cnt);
            total_hash += *hash_cnt;
            free(hash_cnt);
        }
        double wall_end_time = getTime();
        double CPU_end_time = getCPUTime();
        double total_wall_time = wall_end_time - wall_start_time;
        double CPU_ttl_time = CPU_end_time - CPU_start_time;
        // TODO: process results
        int result = 1;
        char temp_pass[9]; 
        strcpy(temp_pass, (const char*) password); // intentional discard volatile
        // printf("%s\n",temp_pass);
        if (strcmp(temp_pass, "") != 0) {
            result = 0;
        }
        v2_print_summary(task->username, temp_pass, total_hash, total_wall_time, CPU_ttl_time, result);
        // mem management
        free((char *) password);
        free(task);
    }
    
    // memory management
    free(null_task);
    free(null_task);
    queue_destroy(q);
    free(threads);
    free(tdata);

    return 0; // DO NOT change the return code since AG uses it to check if your
              // program exited normally
}
