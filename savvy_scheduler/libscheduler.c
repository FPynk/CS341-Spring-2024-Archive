/**
 * savvy_scheduler
 * CS 341 - Spring 2024
 */
#include "libpriqueue/libpriqueue.h"
#include "libscheduler.h"

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "print_functions.h"

// Global varaibles
double total_wait_time;
double total_turn_around_time;
double total_response_time;
double jobs_processed;

/**
 * The struct to hold the information about a given job
 */
typedef struct _job_info {
    int id;

    /* TODO: Add any other information and bookkeeping you need into this
     * struct. */
    double arrival_time;
    double run_time;
    double remaining_time;
    double start_time;
    double priority;
    double time_quantas;
} job_info;

void scheduler_start_up(scheme_t s) {
    switch (s) {
    case FCFS:
        comparision_func = comparer_fcfs;
        break;
    case PRI:
        comparision_func = comparer_pri;
        break;
    case PPRI:
        comparision_func = comparer_ppri;
        break;
    case PSRTF:
        comparision_func = comparer_psrtf;
        break;
    case RR:
        comparision_func = comparer_rr;
        break;
    case SJF:
        comparision_func = comparer_sjf;
        break;
    default:
        printf("Did not recognize scheme\n");
        exit(1);
    }
    priqueue_init(&pqueue, comparision_func);
    pqueue_scheme = s;
    // Put any additional set up code you may need here
    total_wait_time = 0;
    total_response_time = 0;
    total_turn_around_time = 0;
    jobs_processed = 0;
}

static int break_tie(const void *a, const void *b) {
    return comparer_fcfs(a, b);
}

job_info *job_info_ptr(const void *a) {
    return ((job_info *) (((job *) a)->metadata));
}

// -1 if a came first, 1 if b came first, 0 if both came at same time
int comparer_fcfs(const void *a, const void *b) {
    // TODO: Implement me!
    double a_arrival_time = job_info_ptr(a)->arrival_time;
    double b_arrival_time = job_info_ptr(b)->arrival_time;
    if (a_arrival_time < b_arrival_time) {
        return -1;
    } else if (a_arrival_time > b_arrival_time) {
        return 1;
    }
    return 0;
}

int comparer_ppri(const void *a, const void *b) {
    // Complete as is
    return comparer_pri(a, b);
}

int comparer_pri(const void *a, const void *b) {
    // TODO: Implement me!
    double a_priority = job_info_ptr(a)->priority;
    double b_priority = job_info_ptr(b)->priority;
    if (a_priority < b_priority) {
        return -1;
    } else if (a_priority > b_priority) {
        return 1;
    }
    return break_tie(a, b);
}

int comparer_psrtf(const void *a, const void *b) {
    // TODO: Implement me!
    double a_remaining_time = job_info_ptr(a)->remaining_time;
    double b_remaining_time = job_info_ptr(b)->remaining_time;
    if (a_remaining_time < b_remaining_time) {
        return -1;
    } else if (a_remaining_time > b_remaining_time) {
        return 1;
    }
    return break_tie(a, b);
}

int comparer_rr(const void *a, const void *b) {
    // TODO: Implement me!
    double a_time_quantas = job_info_ptr(a)->time_quantas;
    double b_time_quantas = job_info_ptr(b)->time_quantas;
    if (a_time_quantas < b_time_quantas) {
        return -1;
    } else if (a_time_quantas > b_time_quantas) {
        return 1;
    }
    return break_tie(a, b);
}

int comparer_sjf(const void *a, const void *b) {
    // TODO: Implement me!
    double a_run_time = job_info_ptr(a)->run_time;
    double b_run_time = job_info_ptr(b)->run_time;
    if (a_run_time < b_run_time) {
        return -1;
    } else if (a_run_time > b_run_time) {
        return 1;
    }
    return break_tie(a, b);
}

// Do not allocate stack space or initialize ctx. These will be overwritten by
// gtgo
void scheduler_new_job(job *newjob, int job_number, double time,
                       scheduler_info *sched_data) {
    // TODO: Implement me!
    job_info *new_job_info = malloc(sizeof(job_info));

    // set variables
    new_job_info->id = job_number;
    new_job_info->arrival_time = time;
    new_job_info->start_time = -1;
    new_job_info->run_time = sched_data->running_time;
    new_job_info->remaining_time = sched_data->running_time;
    new_job_info->priority = sched_data->priority;
    new_job_info->time_quantas = 0;
    newjob->metadata = (void *) new_job_info;

    // offer to pqueue
    priqueue_offer(&pqueue, newjob);
}
/**
 * This function is called when the quantum timer has expired. It will be called
 * for every scheme.
 *
 * If the last running thread has finished or there were no threads previously
 * running, job_evicted will be NULL.
 *
 * You should return the job* of the next thread to be run. If
 * - there are no waiting threads, return NULL.
 * - the current scheme is non-preemptive and job_evicted is not NULL, return
 *   job_evicted.
 * - the current scheme is preemptive and job_evicted is not NULL, place
 *   job_evicted back on the queue and return the next job that should be ran.
 *
 * @param job_evicted job that is currently running (NULL if no jobs are
 *                    running).
 * @param time the current time.
 * @return pointer to job that should be scheduled, NULL if there are no more
           jobs.
 */
job *scheduler_quantum_expired(job *job_evicted, double time) {
    // TODO: Implement me!
    // NULL job_evicted
    if (job_evicted == NULL) {
        return (job *) priqueue_peek(&pqueue);
    }

    //  update job_info
    job_info *job_evicted_info = job_info_ptr(job_evicted);
    // initialise start time
    if (job_evicted_info->start_time == -1) {
        job_evicted_info->start_time = time - 1;
    }
    // update remaining time and time quantas
    job_evicted_info->remaining_time -= 1;
    job_evicted_info->time_quantas += 1;

    // if queueu is empty return NULL
    if (priqueue_size(&pqueue) <= 0) {
        return NULL;
    }

    // non pre-emptive schemes
    if (pqueue_scheme == FCFS || pqueue_scheme == PRI || pqueue_scheme == SJF) {
        return job_evicted;
    }

    // preemptive: return next and requeue current
    job *next_job = priqueue_poll(&pqueue);
    priqueue_offer(&pqueue, next_job);
    return priqueue_peek(&pqueue);
}
/**
 * Called when a job has completed execution.
 *
 * This function should clean up the metadata and possibly collect information
 * from the thread. Do NOT free job_done.
 *
 * @param job_done pointer to the job that has recenty finished.
 * @param time the current time.
 */
void scheduler_job_finished(job *job_done, double time) {
    // TODO: Implement me!
    // get info
    job_info *job_done_info = job_info_ptr(job_done);
    // update global vars
    ++jobs_processed;
    total_response_time += job_done_info->start_time - job_done_info->arrival_time;
    total_turn_around_time += time - job_done_info->arrival_time;
    total_wait_time += time - job_done_info->arrival_time - job_done_info->run_time;

    // clean up meta data and remove job
    free(job_done->metadata);
    priqueue_poll(&pqueue);
}

static void print_stats() {
    fprintf(stderr, "turnaround     %f\n", scheduler_average_turnaround_time());
    fprintf(stderr, "total_waiting  %f\n", scheduler_average_waiting_time());
    fprintf(stderr, "total_response %f\n", scheduler_average_response_time());
}

double scheduler_average_waiting_time() {
    // TODO: Implement me!
    return total_wait_time/ jobs_processed;
}

double scheduler_average_turnaround_time() {
    // TODO: Implement me!
    return total_turn_around_time / jobs_processed;
}

double scheduler_average_response_time() {
    // TODO: Implement me!
    return total_response_time / jobs_processed;
}

void scheduler_show_queue() {
    // OPTIONAL: Implement this if you need it!
    printf("PRIORITY QUEUE:\n");
    for (int i = 0; i < priqueue_size(&pqueue); ++i) {
        job *curr = priqueue_poll(&pqueue);
        job_info* curr_info = job_info_ptr(curr);
        printf("ID: %d\n", curr_info->id);
        printf("Priority: %f\n", curr_info->priority);
        printf("Arrival Time: %f\n", curr_info->arrival_time);
        printf("Start Time: %f\n", curr_info->start_time);
        printf("Required Time: %f\n", curr_info->run_time);
        printf("Time quantas: %f\n", curr_info->time_quantas);
        printf("Remaining Time: %f\n", curr_info->remaining_time);
        priqueue_offer(&pqueue, curr); // Reinsert the job back into the queue to maintain the state.
    }
}

void scheduler_clean_up() {
    priqueue_destroy(&pqueue);
    print_stats();
}
