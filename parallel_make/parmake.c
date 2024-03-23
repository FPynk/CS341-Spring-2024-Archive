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
        if (set_contains(path, next) || !set_contains(visited, next) && dfs_helper(g, next, visited, path)) {
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

void out_counter_dfs_helper(dictionary *d, graph *g, char *node, set *visited) {
    // base case
    if (node == NULL) { return; }
    set_add(visited, node);
    vector *neighbours = graph_neighbors(g, node);
    size_t n = vector_size(neighbours);
    for (size_t i = 0 ; i < n; ++i) {
        char *next = vector_get(neighbours, i);
        if (set)
        out_counter_dfs_helper()
    }
    vector_destroy(neighbours);
}

void out_counter_dfs(dictionary *d, graph *g, char *node) {
    set *visited = string_set_create();
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
    dictionary *dict = string_to_int_dictionary_create();
    for (size_t i = 0; i < vector_size(goals_clean); ++i) {
        char *goal = vector_get(goals_clean, i);
        out_counter_dfs(dict, d_graph, goal);
    }
    // from lowest to highest (need to track) insert into queue


    // Mem management
    graph_destroy(d_graph);
    vector_destroy(goals);
    vector_destroy(goals_clean);
    return 0;
}
