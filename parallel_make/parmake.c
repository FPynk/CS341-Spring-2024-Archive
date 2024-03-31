/**
 * parallel_make
 * CS 341 - Spring 2024
 */

#include "format.h"
#include "graph.h"
#include "parmake.h"
#include "parser.h"
#include "includes/set.h"
#include "includes/vector.h"
#include "includes/queue.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <time.h>
#include <pthread.h>

// Global vars
static queue *rule_q = NULL;

// PRINT FUNCTIONS
// Helper for debugging
void print_string_vec(vector *vec) {
    if (vec == NULL || vector_size(vec) == 0) {
        printf("The vector is null or empty.\n");
        return;
    }
    
    for (size_t i = 0; i < vector_size(vec); i++) {
        printf("%s\n", (char *) vector_get(vec, i));
    }
}

void D_print_string_vec(vector *vec) {
    #ifdef DEBUG
    print_string_vec(vec);
    #endif
}

void D_print(const char *message) {
    #ifdef DEBUG
    printf("%s", message);
    fflush(stdout); // Immediately flush the output buffer after printing
    #endif
}

void D_print_string_int_dict(dictionary *d) {
    #ifdef DEBUG
    vector *keys = dictionary_keys(d);
    vector *vals = dictionary_values(d);
    if (keys == NULL || vector_size(keys) == 0 || vals == NULL || vector_size(vals) == 0) {
        printf("Dictionary vectors null or empty\n");
    }
    for (size_t i = 0; i < vector_size(keys); ++i) {
        printf("%s: %d\n", (char *) vector_get(keys, i), *((int *) vector_get(vals, i)));
    }
    vector_destroy(keys);
    vector_destroy(vals);
    #endif
}

// BEWARE EDITS QUEUE
void D_print_queue(queue *q) {
    #ifdef DEBUG
    printf("QUEUE CONTENTS:\n");
    char *rule = queue_pull(q);
    while(strcmp(rule, "SENTINEL_VALUE") != 0) {
        printf("%s\n", rule);
        free(rule);
        rule = queue_pull(q);
    }
    printf("%s\n", rule);
    free(rule);
    #endif
}

// Memory:
// Destroy neighboours VECTOR
// Recursive DFS function
// stack used is function call stack
bool dfs_helper(graph* g, char *node, set *visited, set *path) {
    // base case, null
    if (node == NULL) { return false; }
    set_add(visited, node);
    set_add(path, node); // rmmber to back track properly
    vector *neighbours = graph_neighbors(g, node);
    size_t n = vector_size(neighbours);
    for (size_t i = 0 ; i < n; ++i) {
        char *next = vector_get(neighbours, i);
        // check if next node creates cycle
        if (set_contains(path, next) || (!set_contains(visited, next) && dfs_helper(g, next, visited, path))) {
            vector_destroy(neighbours);
            D_print("Cycle detected\n");
            return true;
        }
    }
    set_remove(path, node); // back track
    vector_destroy(neighbours);
    return false;
}
// Memory:
// Destroy visited, path
// Detects cycle in directed graph of starting vertex
// Sets up req sets for path and visited
bool cycle_detect(graph *g, char *start_key) {
    // Sets to store keys
    set *visited = string_set_create(); // Tracks visited nodes
    set *path = string_set_create(); // Tracks nodes in current path, must add and remove as per appropriate
    bool result = dfs_helper(g, start_key, visited, path);
    set_destroy(visited);
    set_destroy(path);
    return result;
}
// Memory:
// Destroy neighbours
void out_counter_dfs_helper(dictionary *d, graph *g, char *node, set *visited) {
    // base case
    if (node == NULL) { return; }
    set_add(visited, node);
    // Get out_deg (arrows going out) and set value in dictionary
    // int *out_deg = malloc(sizeof(int));
    // *out_deg = graph_vertex_degree(g, node);
    int out_deg = graph_vertex_degree(g, node);
    // dictionary_set(d, node, (void *) out_deg);
    dictionary_set(d, node, (void *) &out_deg);
    vector *neighbours = graph_neighbors(g, node);
    size_t n = vector_size(neighbours);
    for (size_t i = 0 ; i < n; ++i) {
        char *next = vector_get(neighbours, i);
        // recurse as per needed
        if (!set_contains(visited, next)) {
            out_counter_dfs_helper(d, g, next, visited);
        }
    }
    vector_destroy(neighbours);
}
// Memory:
// Destroy visited
// Counts out degrees (dependencies of each rule)
// returns max_out , cumulative dependency
void out_counter_dfs(dictionary *d, graph *g, char *node) {
    set *visited = string_set_create(); // destroy
    out_counter_dfs_helper(d, g, node, visited);
    set_destroy(visited);
}

bool is_file(char *rule) {
    return access(rule, F_OK) == 0 ? 1 : 0;
}

// return false if all dependencies are files and NONE are modified later
// return true if a dependency is NOT a file or if a file dependency is modified later
bool file_dependency_check(rule_t *rule_data, graph *g) {
    struct stat target_stat;
    if (stat(rule_data->target, &target_stat) != 0) {
        perror("Failed to get target file stats");
        return false;
    }
    vector *dependencies = graph_neighbors(g, rule_data->target); // destroy
    for (size_t j = 0; j < vector_size(dependencies); ++j) {
        char *dependency = vector_get(dependencies, j);
        // check depency is file
        if (!is_file(dependency)) {
            vector_destroy(dependencies);
            return true;
        } else {
            // get stat, return true if modified later by 1s
            struct stat dep_stat;
            if (stat(dependency, &dep_stat) != 0) {
                perror("Failed to get dependency file stats");
                continue;
            }
            if (difftime(dep_stat.st_mtime, target_stat.st_mtime) > 0) {
                //printf("dep file has later mod time\n");
                return true;
            }
            //printf("dep file has sooner or same mod time\n");
        }
    }
    // all dependents satisfied
    vector_destroy(dependencies);
    return false;
}

// attempts to run all commands in the vector, returns -1 if any failed and 0 if all success
int run_cmds(vector *cmd_vec) {
    if (cmd_vec == NULL || vector_size(cmd_vec) == 0) {
        D_print("cmd_vec is NULL or empty\n");
        return 0;
    }
    int result = 0;
    for (size_t i = 0; i < vector_size(cmd_vec) && result == 0; ++i) {
        result = system((char *) vector_get(cmd_vec, i));
    }
    return result;
}

int can_satisfy(rule_t *rule_data, graph *g) {
    char *rule = rule_data->target;
    vector *dependencies = graph_neighbors(g, rule); // destroy
    for (size_t j = 0; j < vector_size(dependencies); ++j) {
        char *dependency = vector_get(dependencies, j);
        rule_t *dependency_data = graph_get_vertex_value(g, dependency);
        // if dependent not satisfied return its status
        if (dependency_data->state != 1) {
            // if not yet touched re q to get touched
            if (dependency_data->state == 0) {
                // queue_push(rule_q, strdup(dependency));
                queue_push(rule_q, dependency);
            }
            vector_destroy(dependencies);
            return dependency_data->state;
        }
    }
    // all dependents satisfied
    vector_destroy(dependencies);
    return 1;
}

int exec_rule(rule_t *rule_data) {
    vector *cmds = rule_data->commands;
    int status = run_cmds(cmds);
    if (status == 0) {
        rule_data->state = 1;
    } else if (status == -1) {
        rule_data->state = -1;
    } else {
        D_print("Sanity check failed status");
    }
    return rule_data->state;
}

void fill_queue(dictionary *dict, vector *keys, graph *d_graph) {
    while(!dictionary_empty(dict)) {
        for (size_t i = 0; i < vector_size(keys); ++i) {
            char *key = vector_get(keys, i);
            // check key in dict, check val == 0
            if (dictionary_contains(dict, key) && *((int *) dictionary_get(dict, key)) == 0) {
                // push rule to q, update dict
                // MUST FREE RULES FROM QUEUE ONCE PULLED, DYNAMICALLY ALLOCATED
                char *rule = strdup(key);
                queue_push(rule_q, rule);
                // printf("pushed %s to q\n", key);
                // Note: if looking at goal will return vector of size 1 with ""
                vector *dependents = graph_antineighbors(d_graph, key); // destroy
                // D_print("Dependents vec:\n");
                // D_print_string_vec(dependents);
                for (size_t j = 0; j < vector_size(dependents); ++j) {
                    char *dependent = vector_get(dependents, j);
                    // check (to deal with avoiding "")
                    if (dictionary_contains(dict, dependent)) {
                        // printf("updating %s\n", dependent);
                        // grab dependency count and decrement
                        key_value_pair kv = dictionary_at(dict, dependent);
                        *((int *)(*kv.value)) -= 1;
                        // int count = *((int *)(*kv.value));
                        // printf("Updated %s to %d\n", dependent, count);
                    }
                }
                dictionary_remove(dict, key);
                vector_erase(keys, i);
                vector_destroy(dependents);
            }
        }
    }
    // dynamically allocated so all rules are dynamically allocated
    char *sentinel_value = malloc(sizeof(char) * strlen("SENTINEL_VALUE") + 1);
    strcpy(sentinel_value, "SENTINEL_VALUE");
    queue_push(rule_q, sentinel_value);
}

void attempt_satisfy_rule(char *rule, graph *d_graph) {
    // execute rule, update state: -1 failed/not satisfied, 0 not touched, 1 satisfied
    // -1: mark current rule as -1 and do not re add to queue
    // 0: re-add dependency to queue (should not happen)
    // 1: continue to run rule
    // V2 check satisfaction
    rule_t *rule_data = graph_get_vertex_value(d_graph, rule);
    int s_status = can_satisfy(rule_data, d_graph);
    if (s_status == 1) {
        // check if is file
        if (is_file(rule)) {
            //printf("rule is file\n");
            // check if all deps are files and if any has newer mod time
            if(file_dependency_check(rule_data, d_graph)) {
                //printf("file exe\n");
                exec_rule(rule_data);
            } else {
                // if all files and all older, mark as done
                rule_data->state = 1;
            }
        } else {
            // execute rule
            exec_rule(rule_data); // sets state
        }
    } else if (s_status == 0) {
        D_print("A dependency was not touched, we should not be here\n");
        queue_push(rule_q, rule);
    } else if (s_status == -1) {
        rule_data->state = 1;
    } else { D_print("s_status undefined value\n"); }
}

// Numthread worker threads to satisfy rules
// Will: Pull from queue, try to satisfy, shutdown once sentinel value encountered
void *thread_rule_satisfy(void * args) {
    printf("Test thread running\n");
    return NULL;
}

// memory todos
// Notes: goal clean is deep copy of goals, push_back makes deep copies
// Need to free?: 
// Likely need to free: 
// Must free: d_graph, goals, goals_clean
int parmake(char *makefile, size_t num_threads, char **targets) {
    // good luck!
    // Me at start: Fuck
    // Get graph and vector of goal rules
    // graph has strings as keys and rule_t values
    graph *d_graph = parser_parse_makefile(makefile, targets);
    vector *goals = graph_neighbors(d_graph, "");
    // Working 8am Sat
    // print_string_vec(goals);
    // Conduct DFS on vector 
    vector *goals_clean = string_vector_create(); // store goals with no cycles
    for (size_t i = 0; i < vector_size(goals); ++i) {
        char *key = vector_get(goals, i);
        // Detect cycle
        bool cycle = cycle_detect(d_graph, key);
        if (!cycle) {
            D_print("No Cycle\n");
            vector_push_back(goals_clean, key);
        } else {
            print_cycle_failure(key);
        }
    }
    D_print_string_vec(goals_clean);

    // process clean goals
    // Topological sort: Dictionary key: rule val: out degrees
    // BEWARE: less dependencies =/= execute first, only true for 0
    dictionary *dict = string_to_int_dictionary_create(); // destroy this
    for (size_t i = 0; i < vector_size(goals_clean); ++i) {
        char *goal = vector_get(goals_clean, i);
        out_counter_dfs(dict, d_graph, goal);
    }
    D_print("Dictionary:\n");
    D_print_string_int_dict(dict);
    // insert all 0 dependencies
    // once task complete, search for anti neighbours (rules depending on that node)
    // Update dictionary values for those neighbours
    // repeat pushing all 0 dependencies
    rule_q = queue_create(-1); // destroy
    vector *keys = dictionary_keys(dict); // destroy this
    D_print("Dict keys:\n");
    D_print_string_vec(keys);

    // start threads
    pthread_t threads[num_threads];
    for (size_t i = 0; i < num_threads; ++i) {
        if (pthread_create(&threads[i], NULL, thread_rule_satisfy, (void *) d_graph)) {
            perror("failed to create thread\n");
            exit(1);
        }
    }

    // FILL QUEUE AND SENTINEL VALUE
    // this will need to change: Instead of updating -- when queued a rule, must check dependency satisfaction
    // to determine count of each key, only 0 then push
    fill_queue(dict, keys, d_graph);

    // D_print_queue(rule_q); // prevenets mem errors while queue reaches end of code without emptying

    // TODO MOVE TO THREADS
    // executing rules in q
    char *rule = queue_pull(rule_q);
    while(strcmp(rule, "SENTINEL_VALUE") != 0) {
        // attempt to satisfy the rule
        attempt_satisfy_rule(rule, d_graph);
        // Mem manage and pull new queue
        free(rule);
        rule = queue_pull(rule_q);
    }
    free(rule);
    // Thread joining
    for (size_t i = 0; i < num_threads; ++i) {
        if (pthread_join(threads[i], NULL)) {
            perror("failed to join thread\n");
        }
    }

    // Mem management
    graph_destroy(d_graph);
    vector_destroy(goals);
    vector_destroy(goals_clean);
    dictionary_destroy(dict);
    vector_destroy(keys);
    queue_destroy(rule_q);
    return 0;
}
