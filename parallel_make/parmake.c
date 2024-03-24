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
#include <string.h>

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

// memory todos
// Notes: goal clean is deep copy of goals, push_back makes deep copies
// Need to free?: 
// Likely need to free: 
// Must free: d_graph, goals, goals_clean

// Global vars
static queue *rule_q = NULL;

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
    while(!dictionary_empty(dict)) {
        for (size_t i = 0; i < vector_size(keys); ++i) {
            char *key = vector_get(keys, i);
            // check key in dict, check val == 0
            if (dictionary_contains(dict, key) && *((int *) dictionary_get(dict, key)) == 0) {
                // push rule to q, update dict
                // MUST FREE RULES FROM QUEUE ONCE PULLED, DYNAMICALLY ALLOCATED
                char *rule = strdup(key);
                queue_push(rule_q, rule);
                printf("pushed %s to q\n", key);
                // Note: if looking at goal will return vector of size 1 with ""
                vector *dependents = graph_antineighbors(d_graph, key); // destroy
                // D_print("Dependents vec:\n");
                // D_print_string_vec(dependents);
                for (size_t j = 0; j < vector_size(dependents); ++j) {
                    char *dependent = vector_get(dependents, j);
                    // check (to deal with avoiding "")
                    if (dictionary_contains(dict, dependent)) {
                        printf("updating %s\n", dependent);
                        // grab dependency count and decrement
                        key_value_pair kv = dictionary_at(dict, dependent);
                        *((int *)(*kv.value)) -= 1;
                        int count = *((int *)(*kv.value));
                        printf("Updated %s to %d\n", dependent, count);
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
    D_print_queue(rule_q);
    // Mem management
    graph_destroy(d_graph);
    vector_destroy(goals);
    vector_destroy(goals_clean);
    dictionary_destroy(dict);
    vector_destroy(keys);
    queue_destroy(rule_q);
    return 0;
}
