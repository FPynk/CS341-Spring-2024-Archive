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

bool dfs_helper() {
    return false;
}

// Detects cycle in directed graph of starting vertex
bool cycle_detect(graph *g, char *start_key) {
    return false;
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
        bool cycle = false; // place holder
        if (!cycle) {
            vector_push_back(goals_clean, key);
        }
    }
    print_string_vec(goals_clean);

    // Mem management
    graph_destroy(d_graph);
    vector_destroy(goals);
    vector_destroy(goals_clean);
    return 0;
}
