/**
 * finding_filesystems
 * CS 341 - Spring 2024
 */
#include "minixfs.h"
#include "minixfs_utils.h"
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#define D_BLK_SIZE sizeof(data_block)

/**
 * Virtual paths:
 *  Add your new virtual endpoint to minixfs_virtual_path_names
 */
char *minixfs_virtual_path_names[] = {"info", /* add your paths here*/};

/**
 * Forward declaring block_info_string so that we can attach unused on it
 * This prevents a compiler warning if you haven't used it yet.
 *
 * This function generates the info string that the virtual endpoint info should
 * emit when read
 */
static char *block_info_string(ssize_t num_used_blocks) __attribute__((unused));
static char *block_info_string(ssize_t num_used_blocks) {
    char *block_string = NULL;
    ssize_t curr_free_blocks = DATA_NUMBER - num_used_blocks;
    asprintf(&block_string,
             "Free blocks: %zd\n"
             "Used blocks: %zd\n",
             curr_free_blocks, num_used_blocks);
    return block_string;
}

// Don't modify this line unless you know what you're doing
int minixfs_virtual_path_count =
    sizeof(minixfs_virtual_path_names) / sizeof(minixfs_virtual_path_names[0]);

// Helpers
// returns pointer to the data block in the data section of the fs for a given inode
// takes in the no of blocks offset (idx) from the data_block root
// and the offset within the data_block itself
void *get_datablock_ptr(file_system *fs, inode *node, uint64_t idx, uint64_t offset) {
    data_block_number *block;
    if (idx < NUM_DIRECT_BLOCKS) {
        block = node->direct;
    } else {
        block = (data_block_number *) (fs->data_root + node->indirect);
        idx -= NUM_DIRECT_BLOCKS;
    }
    // careful of triple offset calculation here
    void *out = ((void *) (fs->data_root + block[idx] )) + offset;
    return out;
}

int minixfs_chmod(file_system *fs, char *path, int new_permissions) {
    // Thar she blows!
    // get inode for path
    inode *node = get_inode(fs, path);
    // if inode exists:
    if (node) {
        // Get current mode and file type bits
        uint16_t curr_mode = node->mode;
        // combine new perms with extracted bits to get new mode
        // Shifts mode 9 bits to right to get file type, add 9 0 bits to perform | op
        node->mode = new_permissions | ((curr_mode >> RWX_BITS_NUMBER) << RWX_BITS_NUMBER);
        // update c time
        clock_gettime(CLOCK_REALTIME, &node->ctim);
        // return success
        return 0;
    }
    
    // if DNE, set errno and return failure
    errno = ENOENT;
    return -1;
}

int minixfs_chown(file_system *fs, char *path, uid_t owner, gid_t group) {
    // Land ahoy!
    // get inode
    inode *node = get_inode(fs, path);
    // check node exits
    if (node) {
        // check valid owner, not -1
        // if valid update
        if (owner != (uid_t)-1) {
            node->uid = owner;
        }
        // check valid group, not -1
        // if valid update
        if (group != (uid_t)-1) {
            node->gid = group;
        }
        // update ctim 
        clock_gettime(CLOCK_REALTIME, &node->ctim);
        // return success
        return 0;
    }
    // node DNE, errno set and return failure
    errno = ENOENT;
    return -1;
}

inode *minixfs_create_inode_for_path(file_system *fs, const char *path) {
    // Land ahoy!
    // Check if file already exists
    inode *node = get_inode(fs, path);
    // File exists, cannot create new one
    if (node) {
        return NULL;
    }
    // get parent directory, filename
    const char *filename;
    inode *parent = parent_directory(fs, path, &filename);
    // check valid filename
    if (filename == NULL || valid_filename(filename) != 1) {
        return NULL;
    }
    // check is dir
    if (parent == NULL || !is_directory(parent)) {
        return NULL;
    }
    // Find unused inode in the filesystem
    inode_number node_idx = first_unused_inode(fs);
    if (node_idx == -1) {
        return NULL;
    }
    // get reference to the new inode using number
    inode *new_node = fs->inode_root + node_idx;
    // initialise new inode with default settings
    init_inode(parent, new_node);
    // prepare directory entry for the new file
    minixfs_dirent new_dir;
    new_dir.name = (char *) filename;
    new_dir.inode_num = node_idx;
    // find where to put the new directory entry in the parent directory
    // need helper
    uint64_t size = parent->size;
    // no of blocks
    uint64_t n_blocks = size / D_BLK_SIZE;
    // offset within the block
    uint64_t offset = size % D_BLK_SIZE;
    if (n_blocks >= NUM_DIRECT_BLOCKS) {
        return NULL; // idk what to do if its in indirect but lets just not care for now
    }
    if (offset == 0 && (add_data_block_to_inode(fs, parent) == -1)) {
        return NULL;
    }
    void *block_ptr = get_datablock_ptr(fs, parent, n_blocks, offset);
    // write the directory entry into the parent directory
    memset(block_ptr, 0, FILE_NAME_ENTRY);
    make_string_from_dirent(block_ptr, new_dir);
    // update parent directory's size to account for the new entry
    parent->size += FILE_NAME_ENTRY;
    // return newly created inode
    return new_node;
}

ssize_t minixfs_virtual_read(file_system *fs, const char *path, void *buf,
                             size_t count, off_t *off) {
    if (!strcmp(path, "info")) {
        // TODO implement the "info" virtual file here
    }
    
    errno = ENOENT;
    return -1;
}

ssize_t minixfs_write(file_system *fs, const char *path, const void *buf,
                      size_t count, off_t *off) {
    // X marks the spot
    return -1;
}

ssize_t minixfs_read(file_system *fs, const char *path, void *buf, size_t count,
                     off_t *off) {
    const char *virtual_path = is_virtual_path(path);
    if (virtual_path)
        return minixfs_virtual_read(fs, virtual_path, buf, count, off);
    // 'ere be treasure!
    return -1;
}
