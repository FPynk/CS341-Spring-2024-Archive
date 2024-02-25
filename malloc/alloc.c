/**
 * malloc
 * CS 341 - Spring 2024
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

typedef struct meta_data {
    size_t size;
    int is_free;
    struct meta_data *next;
    struct meta_data *prev;
#ifdef DEBUG
    unsigned long magic;    
#endif
} meta_data;

// Global vars
static meta_data* head = NULL;
static size_t total_memory_requested = 0;
static size_t total_memory_freed = 0;

#define META_SIZE sizeof(meta_data)

/**
 * Allocate space for array in memory
 *
 * Allocates a block of memory for an array of num elements, each of them size
 * bytes long, and initializes all its bits to zero. The effective result is
 * the allocation of an zero-initialized memory block of (num * size) bytes.
 *
 * @param num
 *    Number of elements to be allocated.
 * @param size
 *    Size of elements.
 *
 * @return
 *    A pointer to the memory block allocated by the function.
 *
 *    The type of this pointer is always void*, which can be cast to the
 *    desired type of data pointer in order to be dereferenceable.
 *
 *    If the function failed to allocate the requested block of memory, a
 *    NULL pointer is returned.
 *
 * @see http://www.cplusplus.com/reference/clibrary/cstdlib/calloc/
 */
void *calloc(size_t num, size_t size) {
    // implement calloc!
    // calc total memory required
    size_t total_size = num * size;
    if (total_size == 0) {
        return NULL;
    }

    // use malloc to alloc mem
    void *ptr = malloc(total_size);
    if (ptr == NULL) {
        // alloc failed
        return NULL;
    }

    // initialise alloced mem to 0
    memset(ptr, 0, total_size);
    
    return ptr;
}

/**
 * Allocate memory block
 *
 * Allocates a block of size bytes of memory, returning a pointer to the
 * beginning of the block.  The content of the newly allocated block of
 * memory is not initialized, remaining with indeterminate values.
 *
 * @param size
 *    Size of the memory block, in bytes.
 *
 * @return
 *    On success, a pointer to the memory block allocated by the function.
 *
 *    The type of this pointer is always void*, which can be cast to the
 *    desired type of data pointer in order to be dereferenceable.
 *
 *    If the function failed to allocate the requested block of memory,
 *    a null pointer is returned.
 *
 * @see http://www.cplusplus.com/reference/clibrary/cstdlib/malloc/
 */
void *malloc(size_t size) {
    // implement malloc!
    if (size <= 0) {
        return NULL;
    }
    total_memory_requested += size;

    // traverse to end and allocate
    meta_data *curr = head;
    meta_data *fit = NULL; // first fit free block

    // traverse to end of list or first fit free block
    while (curr != NULL) {
        if (curr->is_free && curr->size >= size) {
            fit = curr;
            break;
        }
        curr = curr->next;
    }

    // Reuse block
    if (fit) {
        // TODO: Splitting
        // check spltting is possible
        size_t remaining_size = fit->size - size - META_SIZE;
        // check there's some space left
        if (remaining_size > sizeof(meta_data)) {
            // split block
            meta_data *new_block = (meta_data *) ((char *) (fit + 1) + size);
            new_block->size = remaining_size;
            new_block->is_free = 1;
            new_block->next = fit->next;
            new_block->prev = fit;

            // fix linking
            if (fit->next) {
                fit->next->prev = new_block;
            }
            fit->size = size;
            fit->next = new_block;
        }

        fit->is_free = 0;
        return (void *) (fit + 1);
    }

    // alocate mem block including space for meta data
    size_t total_size = size + META_SIZE;
    meta_data *block = sbrk(total_size);
    if (block == (void *) -1) {
        return NULL; // Allocation failed
    }

    // Initialise metadata
    block->size = size;
    block->is_free = 0;
    block->next = NULL;
    block->prev = curr;

    if (curr) {
        curr->next = block; // connect last block to newly alloced block
    } else {
        head = block;   // First block
    }

    return (void *) (block + 1);
}

/**
 * Deallocate space in memory
 *
 * A block of memory previously allocated using a call to malloc(),
 * calloc() or realloc() is deallocated, making it available again for
 * further allocations.
 *
 * Notice that this function leaves the value of ptr unchanged, hence
 * it still points to the same (now invalid) location, and not to the
 * null pointer.
 *
 * @param ptr
 *    Pointer to a memory block previously allocated with malloc(),
 *    calloc() or realloc() to be deallocated.  If a null pointer is
 *    passed as argument, no action occurs.
 */
void free(void *ptr) {
    // implement free!
    // NULL ptr checking
    if (!ptr) {
        return;
    }

    // get metadata, cast ptr to char * then minus META_SIZE
    meta_data *block = (meta_data *) ((char *) ptr - META_SIZE);
    total_memory_freed += block->size;
    // mark block as free
    block->is_free = 1;

    // coalescing with next block if possible
    if (block->next && block->next->is_free) {
        block->size += block->next->size + META_SIZE;
        block->next = block->next->next;
        // relinking next next block to this block, if it exists
        if (block->next) {
            block->next->prev = block;
        }
    }
    // coalesce with previous blocks if possible
    if (block->prev && block->prev->is_free) {
        block->prev->size += block->size + META_SIZE;
        block->prev->next = block->next;
        if (block->next) {
            block->next->prev = block->prev;
        }
    }
}

/**
 * Reallocate memory block
 *
 * The size of the memory block pointed to by the ptr parameter is changed
 * to the size bytes, expanding or reducing the amount of memory available
 * in the block.
 *
 * The function may move the memory block to a new location, in which case
 * the new location is returned. The content of the memory block is preserved
 * up to the lesser of the new and old sizes, even if the block is moved. If
 * the new size is larger, the value of the newly allocated portion is
 * indeterminate.
 *
 * In case that ptr is NULL, the function behaves exactly as malloc, assigning
 * a new block of size bytes and returning a pointer to the beginning of it.
 *
 * In case that the size is 0, the memory previously allocated in ptr is
 * deallocated as if a call to free was made, and a NULL pointer is returned.
 *
 * @param ptr
 *    Pointer to a memory block previously allocated with malloc(), calloc()
 *    or realloc() to be reallocated.
 *
 *    If this is NULL, a new block is allocated and a pointer to it is
 *    returned by the function.
 *
 * @param size
 *    New size for the memory block, in bytes.
 *
 *    If it is 0 and ptr points to an existing block of memory, the memory
 *    block pointed by ptr is deallocated and a NULL pointer is returned.
 *
 * @return
 *    A pointer to the reallocated memory block, which may be either the
 *    same as the ptr argument or a new location.
 *
 *    The type of this pointer is void*, which can be cast to the desired
 *    type of data pointer in order to be dereferenceable.
 *
 *    If the function failed to allocate the requested block of memory,
 *    a NULL pointer is returned, and the memory block pointed to by
 *    argument ptr is left unchanged.
 *
 * @see http://www.cplusplus.com/reference/clibrary/cstdlib/realloc/
 */
void *realloc(void *ptr, size_t size) {
    // implement realloc!
    // NULL ptr behave like malloc
    if (!ptr) {
        return malloc(size);
    }
    // if size 0, behave like free
    if (size == 0) {
        free(ptr);
        return NULL;
    }

    // Access the block's metadata
    meta_data *block = (meta_data *) ((char *) ptr - META_SIZE);

    // Size is the same
    if (block->size == size) {
        return ptr;
    }
    // Determine size to copy
    size_t copy_size = block->size < size ? block->size : size;

    // realloc within same block, shifted order of ops
    // free old block
    free(ptr);
    // Alloc new block for the requested size
    void *new_ptr = malloc(size);
    if (!new_ptr) {
        return NULL;
    }
    // copy mem
    if (new_ptr != ptr) {
        memcpy(new_ptr, ptr, copy_size);
    }

    return new_ptr;
}
