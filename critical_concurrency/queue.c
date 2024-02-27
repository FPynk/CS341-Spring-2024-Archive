/**
 * critical_concurrency
 * CS 341 - Spring 2024
 */
#include "queue.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

/**
 * This queue is implemented with a linked list of queue_nodes.
 */
typedef struct queue_node {
    void *data;
    struct queue_node *next;
} queue_node;

struct queue {
    /* queue_node pointers to the head and tail of the queue */
    queue_node *head, *tail;

    /* The number of elements in the queue */
    ssize_t size;

    /**
     * The maximum number of elements the queue can hold.
     * max_size is non-positive if the queue does not have a max size.
     */
    ssize_t max_size;

    /* Mutex and Condition Variable for thread-safety */
    pthread_cond_t cv;
    pthread_mutex_t m;
};

queue *queue_create(ssize_t max_size) {
    /* Your code here */
    queue *q = malloc(sizeof(queue));
    if (q == NULL) { // mem alloc fail
        return NULL;
    }
    q->head = q->tail = NULL;
    q->size = 0;
    q->max_size = max_size;
    pthread_mutex_init(&q->m, NULL);
    pthread_cond_init(&q->cv, NULL);
    return q;
}

void queue_destroy(queue *this) {
    /* Your code here */
    pthread_mutex_lock(&this->m);
    while(this->head != NULL) {
        queue_node *tmp = this->head;
        this->head = this->head->next;
        free(tmp);
    }
    pthread_mutex_unlock(&this->m);
    pthread_mutex_destroy(&this->m);
    pthread_cond_destroy(&this->cv);
    free(this);
}

void queue_push(queue *this, void *data) {
    /* Your code here */
    pthread_mutex_lock(&this->m);
    // wait while queue is full (if it has a max size)
    while(this->max_size > 0 && this->size >= this->max_size) {
        pthread_cond_wait(&this->cv, &this->m);
    }

    // create a new node
    queue_node *node = malloc(sizeof(queue_node));
    node->data = data;
    node->next = NULL;
    if(this->tail == NULL) {
        this->head = this->tail = node;
    } else {
        this->tail->next = node;
        this->tail = node;
    }
    this->size++;
    pthread_cond_signal(&this->cv); // signal in case any pulls are waiting
    pthread_mutex_unlock(&this->m);
}

void *queue_pull(queue *this) {
    /* Your code here */
    pthread_mutex_lock(&this->m);
    // wait while queue is empty
    while (this->head == NULL) {
        pthread_cond_wait(&this->cv, &this->m);
    }

    // remove head of the queue
    queue_node *tmp  = this->head;
    void *data = tmp->data;
    this->head = this->head->next;
    if (this->head == NULL) { // update tail if empty
        this->tail = NULL;
    }
    free(tmp);
    this->size--;
    // max size exists, signal waiting pushes
    if (this->max_size > 0) {
        pthread_cond_signal(&this->cv);
    }
    pthread_mutex_unlock(&this->m);
    return data;
}
