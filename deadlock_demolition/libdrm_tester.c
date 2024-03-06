/**
 * deadlock_demolition
 * CS 341 - Spring 2024
 */
#include "libdrm.h"

#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h> // For sleep()

typedef struct {
    drm_t *first_drm;
    drm_t *second_drm;
    pthread_t *thread_id;
} thread_args;

void *thread_lock_routine(void *args) {
    thread_args *t_args = (thread_args *)args;

    // Attempt to lock the first DRM.
    if (drm_wait(t_args->first_drm, t_args->thread_id) == 1) {
        printf("Thread %lu locked first DRM.\n", (unsigned long)pthread_self());
        sleep(1); // Simulate work

        // Attempt to lock the second DRM.
        if (drm_wait(t_args->second_drm, t_args->thread_id) == 1) {
            printf("Thread %lu locked second DRM.\n", (unsigned long)pthread_self());
            drm_post(t_args->second_drm, t_args->thread_id);
        } else {
            printf("Thread %lu could not lock second DRM. Deadlock prevention worked.\n", (unsigned long)pthread_self());
        }

        drm_post(t_args->first_drm, t_args->thread_id);
    } else {
        printf("Thread %lu could not lock first DRM. Deadlock prevention worked.\n", (unsigned long)pthread_self());
    }

    pthread_exit(NULL);
}

void test_single_thread_lock_unlock() {
    printf("Test: Single Thread Locking and Unlocking\n");
    drm_t *drm = drm_init();
    pthread_t thread_id = pthread_self();
    
    assert(drm_wait(drm, &thread_id) == 1); // Should be able to lock
    assert(drm_post(drm, &thread_id) == 1); // Should be able to unlock
    
    drm_destroy(drm);
    printf("Single Thread Locking and Unlocking: Passed\n");
}

void test_deadlock_detection() {
    printf("Test: Deadlock Detection\n");
    drm_t *drm1 = drm_init();
    drm_t *drm2 = drm_init();

    pthread_t thread1, thread2;
    thread_args args1 = {drm1, drm2, &thread1};
    thread_args args2 = {drm2, drm1, &thread2};

    // Create two threads, each attempting to lock the DRMs in reverse order
    pthread_create(&thread1, NULL, thread_lock_routine, &args1);
    pthread_create(&thread2, NULL, thread_lock_routine, &args2);

    // Wait for both threads to finish
    pthread_join(thread1, NULL);
    pthread_join(thread2, NULL);

    // Clean up
    drm_destroy(drm1);
    drm_destroy(drm2);

    printf("Deadlock Detection: Passed\n");
}

int main() {
    // TODO your tests here
    test_single_thread_lock_unlock();
    test_deadlock_detection();
    return 0;
}
