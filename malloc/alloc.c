/**
 * malloc
 * CS 341 - Spring 2024
 */
#include <stdio.h>
#include <stdint.h> 
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>

typedef struct meta_data {
    size_t size_w_flag; // flag for free 1 if free, in lowest bit
    // void *ptr;
    // in order
    struct meta_data *prev; // always check head and end
    // keeps track of free blocks, may not be in order
    struct meta_data *next_free; // always check head_free and head_end
} meta_data;

// Global vars
static meta_data* head = NULL;
static meta_data* end = NULL;
static meta_data* head_free = NULL;
static meta_data* end_free = NULL;

#define META_SIZE sizeof(meta_data)
#define ALIGNED_META_SIZE align_size(META_SIZE)
#define ALIGNMENT 16
#define MIN_SIZE align_size(META_SIZE) * 2 // min size of any block, includes meta_data
#define IS_FREE_MASK 0x1
#define SIZE_MASK (~IS_FREE_MASK)

// Helper functions
size_t align_size(size_t size);
void *get_mem_ptr(meta_data *block);
void *align_ptr(void *ptr);
void delink_free(meta_data *prev, meta_data *block);
void add_free_end(meta_data *block);
void add_free_head(meta_data *block);
void split_block(meta_data *fit, size_t aligned_size);
meta_data *coalesce_block(meta_data *block);

// Aligns size to macro ALIGNMENT, currently 16
size_t align_size(size_t size) {
    return (size + ALIGNMENT - 1) & ~(ALIGNMENT - 1);
}
// returns pointer to data, aligned
void *get_mem_ptr(meta_data *block) {
    void *mem_ptr = (void *)(block + 1);
    void *aligned_ptr = align_ptr(mem_ptr);
    return aligned_ptr;
}
// returns ptr to mem block aligned
void *align_ptr(void *ptr) {
    // uintptr_t for arithmetic operations
    uintptr_t unaligned_addr = (uintptr_t) ptr;
    uintptr_t alignment_mask = (uintptr_t)(ALIGNMENT - 1);
    uintptr_t aligned_addr = (unaligned_addr + alignment_mask) & ~alignment_mask;
    return (void*)aligned_addr;
}
// delinks block from free list
void delink_free(meta_data *prev, meta_data *block) {
    block->size_w_flag = block->size_w_flag & SIZE_MASK; // clear mask, set to not free
    // adjust end pointers
    if (!block->next_free) {
        // updating end
        end_free = prev;
    }
    // check if head or mid list
    if (prev) {
        // mid list
        prev->next_free = block->next_free;
    } else {
        head_free = block->next_free;
    }
    block->next_free = NULL;
}
//adds to end of free list
void add_free_end(meta_data *block) {
    if (!head_free) {
        head_free = block;
        end_free = block;
        block->next_free = NULL;
    } else {
        end_free->next_free = block; 
        block->next_free = NULL;
        end_free = block;
    }
}
// adds to head of free list
void add_free_head(meta_data *block) {
    if (!head_free) {
        head_free = block;
        end_free = block;
        block->next_free = NULL;
    } else {
        block->next_free = head_free;
        head_free = block;
    }
}

// splits block so that fit only has space for aligned size and excess space is a new block
// aligned_size is assumed to be aligned
// also handles linking for regular list and free list
// will add new smaller block at HEAD of free list
void split_block(meta_data *fit, size_t aligned_size) {
    size_t actual_size = fit->size_w_flag & SIZE_MASK;
    if (actual_size < aligned_size + MIN_SIZE) {
        return; // shouldn't reach but just in case
    }
    // pointer to new smaller block start of meta_data
    meta_data *smol = (meta_data *) (((void *) get_mem_ptr(fit)) + aligned_size);

    // edit fit block and shrink
    fit->size_w_flag = aligned_size; // flag set to 0
    // free list for fit handled by delink_free earlier

    // changing smol properties
    // size calcs
    size_t smol_size = actual_size - aligned_size - ALIGNED_META_SIZE; 
    smol->size_w_flag = smol_size;
    smol->size_w_flag |= IS_FREE_MASK; // mark as free

    // Setting up ptr for smol
    // void *mem_ptr = (void *)(smol + 1);
    // void *aligned_ptr = align_ptr(mem_ptr);

    // linking in regular list, edge case of end, head shouldnt happen
    smol->prev = fit;

    // end edge case: smol is new end
    if (end == fit) {
        end = smol;
    } else {
        // smol is not new end, connect to next block
        // grab block after
        meta_data *next = (meta_data *) (((void *) get_mem_ptr(fit)) + actual_size);
        next->prev = smol;
    }

    // add smol to free list
    add_free_head(smol);
}

// coalesces block in front and behind
// does not add to free list
meta_data *coalesce_block(meta_data *block) {
    // edge case block only one here, skip
    if (head == end && end == block) {
        return block;
    }
    // size of block
    size_t block_size = block->size_w_flag & SIZE_MASK;

    // pointers to prev and next blocks
    meta_data *prev_block = block->prev; // will be null if head
    meta_data *next_block = NULL;
    if (end != block) {
        next_block = (meta_data *) (((void *) get_mem_ptr(block)) + block_size);
    }

    // Handle prev, may need to change end
    // check not null and check free
    if (prev_block && prev_block->size_w_flag & IS_FREE_MASK) {
        // delinking prev_block from free list
        meta_data *prev_prev_block = NULL;
        meta_data *curr = head_free;
        // find prev of prev in free list O(n)
        while (curr != prev_block) {
            prev_prev_block = curr;
            curr = curr->next_free;
        }
        delink_free(prev_prev_block, prev_block);

        // handle prev_block size
        size_t act_size_prev_block = prev_block->size_w_flag & SIZE_MASK;
        // prev owns block
        prev_block->size_w_flag = act_size_prev_block + ALIGNED_META_SIZE + block_size;
        // mark prev as is_free
        prev_block->size_w_flag |= IS_FREE_MASK;
        // if block is end, set new end and return
        if (block == end) {
            end = prev_block;
            return prev_block;
        }
        // if block is not end, link block after to prev_block
        next_block->prev = prev_block;
        // update block to prev so next_block coalesce doesn't die
        block = prev_block;
        block_size = block->size_w_flag & SIZE_MASK;
    }

    // Handle next, may need to change end
    if (next_block && next_block->size_w_flag & IS_FREE_MASK) {
        meta_data *next_block_prev_free = NULL;
        meta_data *curr = head_free;
        // find prev of next_block in free list O(n)
        while (curr != next_block) {
            next_block_prev_free = curr;
            curr = curr->next_free;
        }
        delink_free(next_block_prev_free, next_block);

        // next_block size
        size_t act_size_next_block = next_block->size_w_flag & SIZE_MASK;
        // update block size
        block->size_w_flag = act_size_next_block + ALIGNED_META_SIZE + block_size;
        block->size_w_flag |= IS_FREE_MASK;
        // if next block is end, set new end and return
        if (next_block == end) {
            end = block;
            return block;
        }
        // not end, grab next_next_block and link
        meta_data *next_next_block = (meta_data *) (((void *) get_mem_ptr(next_block)) + act_size_next_block);
        next_next_block->prev = block;
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
    meta_data *prev = NULL; //prev in FREE list

    // traverse to end of list or first fit free block
    while (curr != NULL) {
        int is_free = curr->size_w_flag & IS_FREE_MASK;
        size_t actual_size = curr->size_w_flag & SIZE_MASK;
        if (is_free && actual_size >= size) {
            fit = curr;
            break;
        }
        prev = curr;
        curr = curr->next_free;
    }

    size_t aligned_size = align_size(size);

    // Reuse block, may split
    if (fit) {
        delink_free(prev, fit); // fit is set to not free
        // size of fit block
        size_t actual_size = fit->size_w_flag & SIZE_MASK;
        // split if size of fit is large enough to split = req size + Meta_data + mem size of meta_Data
        if (actual_size >= aligned_size + MIN_SIZE) {
            split_block(fit, aligned_size);
        }
        return get_mem_ptr(fit);
    }
    // Creating new block
    curr = end;

    // alocate mem block including space for meta data
    size_t total_size = aligned_size + align_size(META_SIZE);
    meta_data *block = sbrk(total_size);
    if (block == (void *) -1) {
        return NULL; // Allocation failed
    }
    // void *mem_ptr = (void *)(block + 1);
    // void *aligned_ptr = align_ptr(mem_ptr);
    // Initialise metadata
    block->size_w_flag = aligned_size;
    // block->ptr = aligned_ptr;
    block->prev = end;
    block->next_free = NULL;

    if (!end) {
        head = block;   // First block
    }
    // size_t act_size = block->size_w_flag & SIZE_MASK;
    end = block; // (meta_data *) (((void *) block) + align_size(META_SIZE) + act_size); // update last block

    return get_mem_ptr(block);
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

    // get metadata, cast ptr to char then minus META_SIZE
    meta_data *block = (meta_data *) ((void *) ptr - align_size(META_SIZE));
    // mark block as free
    block->size_w_flag |= IS_FREE_MASK;
    // coalesce the block
    block = coalesce_block(block);
    // add to end of free list
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
    // write(STDOUT_FILENO, "realloc\n", 8);
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

    size_t aligned_size = align_size(size);
    // Access the block's metadata
    meta_data *block = (meta_data *) (((void *) ptr) - align_size(META_SIZE));

    size_t block_size = block->size_w_flag & SIZE_MASK;
    // Size is the same
    if (block_size >= aligned_size && block_size < aligned_size * 2 + MIN_SIZE) {
        return ptr;
    }
    // free(ptr);
    // Determine size to copy
    size_t copy_size = block_size < aligned_size ? block_size : aligned_size;
    // Alloc new block for the requested size
    void *new_ptr = malloc(size);
    if (!new_ptr) {
        return NULL;
    }
    // copy mem
    memcpy(new_ptr, ptr, copy_size);
    free(ptr);
    return new_ptr;
}