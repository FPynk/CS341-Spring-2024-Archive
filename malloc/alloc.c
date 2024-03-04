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
    void *ptr;
    // in order
    struct meta_data *next;
    struct meta_data *prev;
    // keeps track of free blocks, may not be in order
    struct meta_data *next_free;
    struct meta_data *prev_free;
} meta_data;

// Global vars
static meta_data* head = NULL;
static meta_data* end = NULL;
static meta_data* head_free = NULL;
static meta_data* end_free = NULL;

#define META_SIZE sizeof(meta_data)

// Removes block from free list
void free_delink(meta_data *block) {
    if (!block) return;
    // relinking free pointers
    if (block->next_free) {
        // link next to prev
        block->next_free->prev_free = block->prev_free;
    } else {
        // updating end
        end_free = block->prev_free;
    }

    if (block->prev_free) {
        block->prev_free->next_free = block->next_free;
    } else {
        head_free = block->next_free;
    }
    block->next_free = NULL;
    block->prev_free = NULL;
}

// Removes block from regular list
void regular_delink(meta_data *block) {
    if (!block) return;
    // relinking free pointers
    if (block->next) {
        // link next to prev
        block->next->prev = block->prev;
    } else {
        // updating end
        end = block->prev;
    }

    if (block->prev) {
        block->prev->next = block->next;
    } else {
        head = block->next;
    }
    block->next = NULL;
    block->prev = NULL;
}

void add_free_end(meta_data* block) {
    if (!block) return;

    if (!head_free) {
        head_free = block;
        end_free = block;
        block->next_free = NULL;
        block->prev_free = NULL;
    } else {
        end_free->next = block;
        block->prev_free = end_free;
        block->next_free = NULL;
        end_free = block;
    }
}

// splits block into 2 smaller memory blocks
// returns pointer of 1st memory block
// Assume 1st will be used and 2nd is free
// 1st arg is pointer to block to be split
// 2nd arg is size of the smaller block
meta_data *split_block(meta_data *large_block, size_t size) {
    // write(STDOUT_FILENO, "splitting\n", 11);
    meta_data *smol_block = (meta_data *)(((void *)(large_block + 1)) + size);
    
    // Initialize the small block
    smol_block->size = large_block->size - size - META_SIZE; // Correct size calculation
    smol_block->is_free = 1;
    smol_block->ptr = (void *)(smol_block + 1);
    smol_block->next_free = NULL; // Initialize free list pointers
    smol_block->prev_free = NULL;
    
    // Adjust the large block's properties
    large_block->size = size;
    large_block->is_free = 0; // Assuming it will be used immediately
    
    // Adjust the regular doubly linked list pointers
    smol_block->next = large_block->next;
    smol_block->prev = large_block;
    if (large_block->next) {
        large_block->next->prev = smol_block;
    }
    large_block->next = smol_block;

    // Update the global 'end' if necessary
    if (end == large_block) {
        end = smol_block;
    }
    
    free_delink(large_block);
    // Handle the free list
    // No need to delink large_block from the free list here, since it's being used
    // Add smol_block to the free list
    add_free_end(smol_block);

    return large_block; // The first block is returned, now ready to be used
}

// reuses block ,splits if needed
void *reuse_block(meta_data *fit, size_t size) {
    
    size_t remaining_size = fit->size - size - META_SIZE;
    if (remaining_size >= META_SIZE) {
        meta_data *block = split_block(fit, size);
        return block->ptr;
    } else {
        fit->is_free = 0;
        free_delink(fit);
        return fit->ptr;
    }
}

// reuse block, no split
void *reuse_block_old(meta_data *fit) {
    fit->is_free = 0;
    // relinking free pointers
    if (fit->next_free) {
        // link next to prev
        fit->next_free->prev_free = fit->prev_free;
    } else {
        // updating end
        end_free = fit->prev_free;
    }

    if (fit->prev_free) {
        fit->prev_free->next_free = fit->next_free;
    } else {
        head_free = fit->next_free;
    }
    
    fit->next_free = NULL;
    fit->prev_free = NULL;
    return fit->ptr;
}

// coalesce block, 
void *coalesce_block(meta_data* block) {
     // coalesce curr and back
    if (block->prev && block->prev->is_free) {
        // write(STDOUT_FILENO, "coalesce front\n", 15);
        // handle free list delink
        meta_data *prev_block = block->prev;
        free_delink(prev_block);
        free_delink(block);
        // handle regular list delink
        regular_delink(block);

        // handle regular list adjustment
        prev_block->size += block->size + META_SIZE;
        // handle free list relink
        block = prev_block;
    }
    
    // coalesce front and curr
    if (block->next && block->next->is_free) {
        // write(STDOUT_FILENO, "coalesce back\n", 14);
        // handle free list delink
        meta_data *next_block = block->next;
        free_delink(next_block);
        free_delink(block);
        // handle regular list delink
        regular_delink(next_block);

        // handle regular list adjustment
        block->size += META_SIZE + next_block->size;
    }
    return block;
}


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
    // write(STDOUT_FILENO, "malloc\n", 7);
    // implement malloc!
    if (size <= 0) {
        return NULL;
    }

    // traverse to end and allocate
    meta_data *curr = head_free;
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
        return reuse_block(fit, size);
        // return reuse_block_old(fit);
    }
    // Creating new block
    curr = end;

    // alocate mem block including space for meta data
    size_t total_size = size + META_SIZE;
    meta_data *block = sbrk(total_size);
    if (block == (void *) -1) {
        return NULL; // Allocation failed
    }

    // Initialise metadata
    block->size = size;
    block->is_free = 0;
    block->ptr = (void *) (block + 1);
    block->next = NULL;
    block->prev = curr;
    block->next_free = NULL;
    block->prev_free = NULL;

    if (curr) {
        curr->next = block; // connect last block to newly alloced block
    } else {
        head = block;   // First block
    }
    end = block; // update last block

    return block->ptr;
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
    // write(STDOUT_FILENO, "free\n", 5);
    // implement free!
    // NULL ptr checking
    if (!ptr) {
        return;
    }

    // get metadata, cast ptr to char * then minus META_SIZE
    meta_data *block = (meta_data *) ((char *) ptr - META_SIZE);
    // mark block as free
    block->is_free = 1;

    block = coalesce_block(block);

    add_free_end(block);
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
    write(STDOUT_FILENO, "realloc\n", 8);
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
    meta_data *block = (meta_data *) (((void *) ptr) - META_SIZE);

    // Size is the same
    if (block->size >= size) {
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