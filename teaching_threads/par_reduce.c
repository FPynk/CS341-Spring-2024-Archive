/**
 * teaching_threads
 * CS 341 - Spring 2024
 */
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "reduce.h"
#include "reducers.h"

/* You might need a struct for each task ... */
typedef struct {
    int *list;              // pointer to list
    size_t start;           // Start index inclusive
    size_t end;             // end index exclusive
    reducer reduce_func;    // reducer function
    int base_case;          // base case
    int result;             // store result
} ThreadArg;
/* You should create a start routine for your threads. */
void *thread_start_routine(void *arg) {
    ThreadArg *thread_data = (ThreadArg *) arg;
    int result = thread_data->base_case;
    // Cycle thru assigned protion of list and uses func
    for (size_t i = thread_data->start; i < thread_data->end; ++i) {
        result = thread_data->reduce_func(result, thread_data->list[i]);
    }
    thread_data->result = result;       // use this to grab result
    return 0;
}


int par_reduce(int *list, size_t list_len, reducer reduce_func, int base_case,
               size_t num_threads) {
    /* Your implementation goes here */

    // Edge cases: num_threads > list len or list len doesnt divide nicely by num threads
    // General case: num_thread <= list len and divides nicely
    size_t num_threads_act = 0;
    if (num_threads > list_len) {
        num_threads_act = list_len;
    } else {
        num_threads_act = num_threads;
    }

    pthread_t *threads = malloc(num_threads_act * sizeof(pthread_t));
    ThreadArg *thread_args = malloc(num_threads_act * sizeof(ThreadArg));

    size_t start = 0;
    size_t segment_size = list_len / num_threads_act;
    size_t extra_ele = list_len % num_threads_act;

    // cycle thru no of threads, assign appropriate no of eles
    for (size_t i = 0; i < num_threads_act; ++i) {
        // calc end/ no of eles
        size_t end = start + segment_size + (i < extra_ele ? 1 : 0);
        // Explicit definition of thread arg and immediately put into array
        thread_args[i] = (ThreadArg) {.list = list,
                                      .start = start,
                                      .end = end,
                                      .reduce_func = reduce_func,
                                      .base_case = base_case,
                                      .result = 0
                                      };
        // create thread with relevant info
        pthread_create(&threads[i], NULL, thread_start_routine, &thread_args[i]);
        // update start
        start = end;
    }
    int final_result = base_case;
    for (size_t i = 0; i < num_threads_act; ++i) {
        pthread_join(threads[i], NULL);
        final_result = reduce_func(final_result, thread_args[i].result);
    }

    free(threads);
    free(thread_args);

    return final_result;
}
