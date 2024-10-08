/**
 * deadlock_demolition
 * CS 341 - Spring 2024
 */
#include "graph.h"
#include "libdrm.h"
#include "set.h"
#include <pthread.h>
#include <stdio.h>

// Global resource allocation graph
static graph *rag = NULL;
static pthread_mutex_t rag_lock = PTHREAD_MUTEX_INITIALIZER;
// set *visited = NULL;

struct drm_t {
    pthread_mutex_t mutex;
};

// bool is_graph_cyclic(graph *g);
bool is_cyclic_helper(graph *g, void *v, set *visited, set *recStack);

void print_graph_structure(graph *g) {
    vector *vertices = graph_vertices(g); // Retrieve all vertices in the graph
    if (vertices == NULL) {
        printf("Graph is empty or failed to retrieve vertices.\n");
        return;
    }

    for (size_t i = 0; i < vector_size(vertices); ++i) {
        void *vertex = vector_get(vertices, i); // Get the vertex at index i

        // Print the current vertex
        printf("Vertex %p points to: ", vertex); 

        vector *neighbours = graph_neighbors(g, vertex); // Retrieve neighbors of the current vertex
        if (neighbours != NULL && vector_size(neighbours) > 0) {
            for (size_t j = 0; j < vector_size(neighbours); ++j) {
                void *neighbour = vector_get(neighbours, j); // Get the neighbor at index j
                printf("%p ", neighbour); // Print the neighbor
            }
        } else {
            printf("No neighbors.");
        }
        printf("\n");
    }

}

// TODO cyclic detection
bool is_graph_cyclic(graph *g) {
    printf("is_graph_cyclic\n");
    print_graph_structure(g);
    set *visited = shallow_set_create();
    set *recStack = shallow_set_create();

    vector *vertices = graph_vertices(g);
    for (size_t i = 0; i < vector_size(vertices); ++i) {
        void *vertex = vector_get(vertices, i);
        if (!set_contains(visited, vertex) && is_cyclic_helper(g, vertex, visited, recStack)) {
            visited = NULL;
            recStack = NULL;
            printf("deadlock_detected\n");
            return true;
        }
    }
    visited = NULL;
    recStack = NULL;
    printf("no deadlock_detected, end of is_graph_cyclic\n");
    return false;
}

bool is_cyclic_helper(graph *g, void *v, set *visited, set *recStack) {
    // printf("is_cyclic_helper\n");
    if (!set_contains(visited, v)) {
        // mark current node as visited and part of recursion stack
        set_add(visited, v);
        set_add(recStack, v);

        // recur for all vertices adjacent to this vertex
        vector *neighbours = graph_neighbors(g, v);
        for (size_t i = 0; i < vector_size(neighbours); ++i) {
            void *neighbour = vector_get(neighbours, i);
            if (!set_contains(visited, neighbour) && is_cyclic_helper(g, neighbour, visited, recStack)) {
                printf("deadlock_detected first\n");
                return true;
            } else if (set_contains(recStack, neighbour)) {
                printf("deadlock_detected first\n");
                return true;
            }
        }
    }
    // printf("is_cyclic_helper end\n");
    // remove vertex from recursion stack
    set_remove(recStack, v);
    return false;
}

drm_t *drm_init() {
    pthread_mutex_lock(&rag_lock);
    if (rag == NULL) {
        rag = shallow_graph_create();
    }

    drm_t *drm = malloc(sizeof(drm_t));
    if (drm == NULL) {
        pthread_mutex_unlock(&rag_lock);
        return NULL;
    }
    pthread_mutex_init(&drm->mutex, NULL);
    graph_add_vertex(rag, drm);
    pthread_mutex_unlock(&rag_lock);
    return drm;
}

int drm_post(drm_t *drm, pthread_t *thread_id) {
    printf("DRM_POST, drm_id: %p, thread_id: %lu\n", (void *)drm, *thread_id);
    /* Your code here */
    pthread_mutex_lock(&rag_lock);
    printf("drm_wait, rag_locked\n");
    if (!graph_contains_vertex(rag, thread_id) || !graph_contains_vertex(rag, drm)) {
        pthread_mutex_unlock(&rag_lock);
        return 0;
    }
    if (graph_adjacent(rag, drm, thread_id)) {
        graph_remove_edge(rag, drm, thread_id); // Update RAG accordingly.
        pthread_mutex_unlock(&drm->mutex); // Unlock the drm mutex.
    }
    pthread_mutex_unlock(&rag_lock); // Release the global RAG lock.
    printf("drm_wait, rag_unlocked\n");
    return 1; // Indicate success.
}

int drm_wait(drm_t *drm, pthread_t *thread_id) {
    printf("DRM_WAIT, drm_id: %p, thread_id: %lu\n", (void *)drm, *thread_id);
    /* Your code here */
    pthread_mutex_lock(&rag_lock);
    printf("drm_wait, rag_locked by: %p\n", (void *)drm);
    // add thread as vertex if doesn't exist
    if (!graph_contains_vertex(rag, thread_id)) {
        graph_add_vertex(rag, thread_id);
    }

    graph_add_edge(rag, thread_id, drm);

    if (is_graph_cyclic(rag)) {
        printf("deadlock detected if\n");
        graph_remove_edge(rag, thread_id, drm); // STUCK HERE
        printf("after graph remove edge\n");
        pthread_mutex_unlock(&rag_lock);
        printf("drm_wait, rag_unlocked\n");
        return 0; // deadlock will occur do not allow lock
    }
    printf("no deadlock detected if\n");
    // no deadlock, lock drm
    pthread_mutex_unlock(&rag_lock);
    pthread_mutex_lock(&drm->mutex);
    pthread_mutex_lock(&rag_lock);
    graph_remove_edge(rag, thread_id, drm);
    graph_add_edge(rag, drm, thread_id);
    pthread_mutex_unlock(&rag_lock);
    printf("drm_wait, rag_unlocked by: %p\n", (void *)drm);
    return 1;
}

void drm_destroy(drm_t *drm) {
    pthread_mutex_lock(&rag_lock);
    graph_remove_vertex(rag, drm);
    pthread_mutex_destroy(&drm->mutex);
    free(drm);
    pthread_mutex_unlock(&rag_lock);
    return;
}
