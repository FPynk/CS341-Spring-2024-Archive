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

// returns min of int like values
size_t min(size_t a, size_t b) {
    return a > b ? b : a;
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
    // offset within the blockw
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
        // grab data map
        char *data = GET_DATA_MAP(fs->meta);
        uint64_t ttl_blks = fs->meta->dblock_count;
        // Count used and unused data blocks // unused removed, not needed
        size_t n_used = 0;
        // size_t n_not_used = 0;
        for (uint64_t i = 0; i < ttl_blks; ++i) {
            if (data[i]) {
                ++n_used;
            }
            // } else {
            //     ++n_not_used;
            // }
        }
        // Get string to print out, provided helper already prints it for us
        char *info_out = block_info_string(n_used);
        size_t info_len = strlen(info_out);
        // Determine how many bytes of the str we need to read, check offset doesnt exceed info_len
        size_t bytes_read = min(count, info_len - *off);
        // Not exceeded, we have to read stuff
        if (bytes_read > 0) {
            memcpy(buf, info_out + *off, bytes_read);
            *off += bytes_read;
        } else {
            // doesnt make sense to return negative bytes read
            bytes_read = 0;
        }
        return bytes_read;
    }
    
    errno = ENOENT;
    return -1;
}

ssize_t minixfs_write(file_system *fs, const char *path, const void *buf,
                      size_t count, off_t *off) {
    // X marks the spot
    // get inode
    inode *node = get_inode(fs, path);
    // create inode if doesnt exist
    if (node == NULL) {
        node = minixfs_create_inode_for_path(fs, path);
        if (node == NULL) {
            errno = ENOSPC;
            return -1;
        }
    }
    // check we can write all the data to the file, off + count less than file max possible size
    uint64_t max_file_size = (NUM_DIRECT_BLOCKS + NUM_INDIRECT_BLOCKS) * D_BLK_SIZE;
    if (count + *off > max_file_size) {
        errno = ENOSPC;
        return -1;
    }
    // check inode has enough space, blocks allocated, allocate more if necessary
    // deref off  to get bytes, + count bytes and round up with D_BLK_SIZE - 1
    // division to get block number not bytes
    int blocks_required = (*off + count + D_BLK_SIZE - 1) / D_BLK_SIZE;
    // Check success
    if (minixfs_min_blockcount(fs, path, blocks_required) == -1) {
        errno = ENOSPC;
        return -1;
    }
    // writing
    // Calculate initial block idx, and offset inside of block
    uint64_t blk_idx = *off / D_BLK_SIZE;
    uint64_t blk_offset = *off % D_BLK_SIZE;
    // points to last written pos in buffer
    void *buf_ptr = (void *) buf;
    while (count > 0) {
        // loop fills whole blocks for n-1 loops and fills partial block n loop and 1st loop
        // update each loop: size write per iter, offset within block and block index
        // determine size to write in first iter, min of data or space left in block
        uint64_t write_size = min(D_BLK_SIZE - blk_offset, count);
        void *blk_ptr = get_datablock_ptr(fs, node, blk_idx, blk_offset);
        // write buf -> file
        memcpy(blk_ptr, buf_ptr, write_size);
        // update off, count, ptr within buffer, blk_idx
        *off += write_size;
        count -= write_size;
        buf_ptr += write_size;
        blk_idx += 1;
        blk_offset = 0; // non zero only for first loop
    }
    // update filesize, mtim, atim
    // not just added since can overwrite data and size still be same
    // double check file update
    if ((uint64_t) *off > node->size) {
        node->size = *off;
    }
    clock_gettime(CLOCK_REALTIME, &(node->mtim));
    clock_gettime(CLOCK_REALTIME, &(node->atim));
    return buf_ptr - buf; // return bytes written
}

ssize_t minixfs_read(file_system *fs, const char *path, void *buf, size_t count,
                     off_t *off) {
    const char *virtual_path = is_virtual_path(path);
    if (virtual_path)
        return minixfs_virtual_read(fs, virtual_path, buf, count, off);
    // 'ere be treasure!
    // get inode
    inode *node = get_inode(fs, path);
    // create inode if doesnt exist
    if (node == NULL) {
        errno = ENOENT;
        return -1;
    }
    // check read is within data size
    if ((uint64_t) *off > node->size) {
        return 0;
    }
    
    // reading
    // Calculate initial block idx, and offset inside of block
    uint64_t blk_idx = *off / D_BLK_SIZE;
    uint64_t blk_offset = *off % D_BLK_SIZE;
    // points to last written pos in buffer
    void *buf_ptr = (void *) buf;
    // adjust count so we only read up to EOF
    count = min(count, node->size - *off); 
    while (count > 0) {
        // loop fills whole blocks for n-1 loops and fills partial block n loop and 1st loop
        // update each loop: size read per iter, offset within block and block index
        // determine size to read in first iter, min of data or space left in block
        uint64_t read_size = min(D_BLK_SIZE - blk_offset, count);
        void *blk_ptr = get_datablock_ptr(fs, node, blk_idx, blk_offset);
        // write fle -> buf, reading form file
        memcpy(buf_ptr, blk_ptr, read_size);
        // update off, count, ptr within buffer, blk_idx
        *off += read_size;
        count -= read_size;
        buf_ptr += read_size;
        blk_idx += 1;
        blk_offset = 0; // non zero only for first loop
    }
    // update filesize atim
    clock_gettime(CLOCK_REALTIME, &(node->atim));
    return buf_ptr - buf; // return bytes read
}