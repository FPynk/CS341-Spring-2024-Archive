/**
 * password_cracker
 * CS 341 - Spring 2024
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cracker1.h"
#include "format.h"
#include "utils.h"
#include "includes/queue.h"

// static char *salt = "xx";

typedef struct {
    char username[9]; // names 8 char + \0
    char password_hash[14]; // hashes 13 chars + \0
    char known_part[9]; // known part + unknown part 8 chars + \0
} task_details;

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
    // DEBUG: Print out contents of queue to ensure all details added correctly
    task_details *curr_task = (task_details *) queue_pull(q); // pull, cast and deref
    while (strcmp(curr_task->username, null_task->username)) {
        printf("%s %s %s\n", curr_task->username, curr_task->password_hash, curr_task->known_part);
        free(curr_task);
        curr_task = (task_details *) queue_pull(q);
    }

    // TODO: Create and manage threads to process each task

    // memory management
    free(curr_task);
    queue_destroy(q);
    return 0; // DO NOT change the return code since AG uses it to check if your
              // program exited normally
}
