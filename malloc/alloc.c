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
    void *ptr;
    // in order
    struct meta_data *prev;
    // keeps track of free blocks, may not be in order
    struct meta_data *next_free;
} meta_data;

// Global vars
static meta_data* head = NULL;
static meta_data* end = NULL;
static meta_data* head_free = NULL;
static meta_data* end_free = NULL;

#define META_SIZE sizeof(meta_data)
#define ALIGNMENT 16
#define MIN_SIZE align_size(META_SIZE) * 2 // min size of any block, includes meta_data
#define IS_FREE_MASK 0x1
#define SIZE_MASK (~IS_FREE_MASK)

// Helper functions
size_t align_size(size_t size);
void *align_ptr(void *ptr);
void delink_free(meta_data *prev, meta_data *block);
void add_free_end(meta_data *block);

// Aligns size to macro ALIGNMENT, currently 16
size_t align_size(size_t size) {
    return (size + ALIGNMENT - 1) & ~(ALIGNMENT - 1);
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
    meta_data *prev = NULL;

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

    // Reuse block
    if (fit) {
        delink_free(prev, fit);
        return (void *) (fit->ptr);
    }
    // Creating new block
    curr = end;

    // alocate mem block including space for meta data
    size_t aligned_size = align_size(size);
    size_t total_size = aligned_size + align_size(META_SIZE);
    meta_data *block = sbrk(total_size);
    if (block == (void *) -1) {
        return NULL; // Allocation failed
    }
    void *mem_ptr = (void *)(block + 1);
    void *aligned_ptr = align_ptr(mem_ptr);
    // Initialise metadata
    block->size_w_flag = aligned_size;
    // block->size_w_flag &= SIZE_MASK;
    block->ptr = aligned_ptr;
    block->prev = end;
    block->next_free = NULL;

    if (!end) {
        head = block;   // First block
    }
    // size_t act_size = block->size_w_flag & SIZE_MASK;
    end = block; // (meta_data *) (((void *) block) + align_size(META_SIZE) + act_size); // update last block

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

    // get metadata, cast ptr to char then minus META_SIZE
    meta_data *block = (meta_data *) ((void *) ptr - align_size(META_SIZE));
    // mark block as free
    block->size_w_flag |= IS_FREE_MASK;

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
    if (block_size >= aligned_size) {
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