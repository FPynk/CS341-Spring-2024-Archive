#include <pthread.h>

static pthread_mutex_t A;
static pthread_mutex_t B;

void *thread1_function(void *arg) {
    pthread_mutex_lock(&A);
    pthread_mutex_lock(&B);

    // Critical section code that involves both resources protected by A and B
    // Example operations
    printf("Thread 1 is running\n");

    pthread_mutex_unlock(&B);
    pthread_mutex_unlock(&A);
    return NULL;
}

void *thread2_function(void *arg) {
    pthread_mutex_lock(&A);
    pthread_mutex_lock(&B);

    // Critical section code that involves both resources protected by A and B
    // Example operations
    printf("Thread 2 is running\n");

    pthread_mutex_unlock(&B);
    pthread_mutex_unlock(&A);

    return NULL;
}

int main() {
    pthread_t thread1, thread2;

    pthread_mutex_init(&A, NULL);
    pthread_mutex_init(&B, NULL);

    pthread_create(&thread1, NULL, thread1_function, NULL);
    pthread_create(&thread2, NULL, thread2_function, NULL);

    pthread_join(thread1, NULL);
    pthread_join(thread2, NULL);

    pthread_mutex_destroy(&A);
    pthread_mutex_destroy(&B);

    return 0;
}



















#include <pthread.h>
#include <stdio.h>

#define BUFFER_SIZE 10
int buffer[BUFFER_SIZE];
int count = 0;

pthread_mutex_t mutex;
pthread_cond_t cond;


void *producer(void *arg) {
    for (int i = 0; i < 100; i++) {
        pthread_mutex_lock(&mutex);
        while (count == BUFFER_SIZE) {
            // wait for sapce
            pthread_cond_wait(&cond, &mutex);
        }
        // add
        buffer[count++] = i;
        printf("Produced: %d\n", i);
        // signal to consumer data available
        pthread_cond_signal(&cond);
        pthread_mutex_unlock(&mutex);
    }
    return NULL;
}


void *consumer(void *arg) {
    for (int i = 0; i < 100; i++) {
        pthread_mutex_lock(&mutex);
        while (count == 0) {
            // wait for stuff in buffer
            pthread_cond_wait(&cond, &mutex); // releases mutex, wait on cond
        }
        // remove
        int item = buffer[--count];
        printf("Consumed: %d\n", item);
        // signal to producer space available
        pthread_cond_signal(&cond); // wake up any thread waiting on cond
        pthread_mutex_unlock(&mutex);
    }
    return NULL;
}

int main() {
    pthread_t prod, cons;

    pthread_mutex_init(&mutex, NULL);
    pthread_cond_init(&cond, NULL);

    pthread_create(&prod, NULL, producer, NULL);
    pthread_create(&cons, NULL, consumer, NULL);

    pthread_join(prod, NULL);
    pthread_join(cons, NULL);

    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&cond);

    return 0;
}