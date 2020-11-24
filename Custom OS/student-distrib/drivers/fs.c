#include "fs.h"

#include "../tasks.h"
#include "../types.h"
#include "../lib.h"

#define MAX_NAME_LENGTH 32

int32_t* fs_op_table[4] = {(int32_t*)file_open, (int32_t*)file_read, (int32_t*)file_write, (int32_t*)file_close};

static boot_block_t* b_block;
static inode_t* inodes;
static d_block_t* d_blocks;
static uint32_t fs_end;

/*
filesystem_init
Description: initialize file system by setting up pointers
Input: pointer to start of filesystem module
Output: None
*/
int32_t filesystem_init(uint32_t module_start, uint32_t module_end) {
    // filter bad input
    if (!module_start || !module_end) return -1;
    b_block = (boot_block_t*) module_start;
    inodes = (inode_t*) (module_start + BLOCK_SIZE);
    d_blocks = (d_block_t*) (module_start + BLOCK_SIZE + (b_block->num_inodes*BLOCK_SIZE));
    fs_end = module_end;
    return 0;
}
/*
read_dentry_by_name
Description: fills dentry block with dentry in fs with specified name
Input: name buffer pointer, dentry block to fill out
Output: 0 on success, -1 on fail (non-existent)
Effect: dentry filled with corresponding directory entry in fs
*/
int32_t read_dentry_by_name(const uint8_t * fname, dentry_t* dentry) {
    uint32_t i, j;
    for (i = 0; i < b_block->num_dentries; i++) {
        for (j = 0; j < MAX_NAME_LENGTH; j++) {
            // mismatch string, try next
            if(fname[j] != (b_block->dentries[i]).f_name[j])
                break;
            // fname EOS, or all 32 characters match
            if(fname[j] == 0 || j == MAX_NAME_LENGTH-1)
                goto FOUND;
        }
    }
    // file not found
    return -1;
    // file found, copy dentry data
    FOUND:
    return read_dentry_by_index(i, dentry);
}
/*
read_dentry_by_index
Description: fills dentry block with dentry in fs with specified index
Input: index, dentry block to fill out
Output: 0 on success, -1 on fail (index out of bound)
Effect: dentry filled with corresponding directory entry in fs
*/
int32_t read_dentry_by_index(uint32_t index, dentry_t* dentry) {
    uint32_t j;
    // index not in range specified by b_block
    if (index >= b_block->num_dentries) return -1;
    // file found, copy dentry data
    memset(dentry->f_name, 0, MAX_NAME_LENGTH); // clear f_name
    for(j = 0; j < NAME_LENGTH; j++) {
        dentry->f_name[j] = (b_block->dentries[index]).f_name[j];
    }
    dentry->f_type = (b_block->dentries[index]).f_type;
    dentry->inode_index = (b_block->dentries[index]).inode_index;
    return 0;
}
/*
read_data
Description: read data for file specified by inode
Input: file specified by inode, starting offset, buffer data should be read into, length to be read
Output: number of bytes read. if read till EOF, 0
Effects: reads data into buf
*/
int32_t read_data(uint32_t inode, uint32_t offset, uint8_t* buf, uint32_t length) {
    uint32_t i, b_read, j;
    uint32_t file_size = inodes[inode].length;

    // filter inode input
    if (inode > b_block->num_inodes) return -1;
    //printf("inode: %d f_size: %d",inode, file_size);

    b_read = 0;
    // start at block offset is in, end at block the file ends at
    for (i = (offset/BLOCK_SIZE); i <= (file_size/BLOCK_SIZE); i++) {
        // reading our first block, start j at offset in block
        // if reading any other block, start j at 0
        j = (!b_read) ? offset % BLOCK_SIZE : 0;
        // read up to BLOCK_SIZE per block
        for ( ; j < BLOCK_SIZE; j++) {
            if (b_read >= length || b_read >= (file_size-offset)) goto DONE;
            buf[b_read++] = d_blocks[inodes[inode].db_index[i]].data[j];
        }
    }
    DONE:
    // return bytes read
    return b_read;
}
/*
read_directory
Desc: Handle reading of directory type entry
Input: Buf to write entry name to and nbytes to write
Output: number of bytes read, or 0 if no more files in directory to read
*/
int32_t read_directory(uint32_t index, void* buf, int32_t nbytes) {
    // wipe buf
    memset(buf,0,nbytes);
    uint8_t* filename = b_block->dentries[index].f_name;
    int i;
    // put name of enteries in buf
    for (i = 0; i < MAX_NAME_LENGTH && i < nbytes && filename[i] != 0; i++) {
        ((uint8_t*) buf)[i] = filename[i];
    }
    return i;
}
/*
file_opens
Description: opens file and puts on top of file stack
Input: filename
Output: 0 on success, -1 on non-existent file
Effects: pushes opened file on top of stack
*/
int32_t file_open(int32_t fd, const uint8_t* filename) {
    // filter fd
    if (fd < 2) return -1;
    // open dentry and put dentry in opened file
    dentry_t open_dentry;
    int32_t ret = read_dentry_by_name(filename, &open_dentry);
    if (ret == 0) {
        current_task_pcb->fd_arr[fd].inode = open_dentry.inode_index;
        current_task_pcb->fd_arr[fd].file_position = 0;
    }
    return ret;
}
/*
file_read
Description: read data for file currently open
Input: buffer data should be read into, length to be read
Output: number of bytes read. if read till EOF, 0
Effects: reads data into buf
*/
int32_t file_read(int32_t fd, void* buf, int32_t nbytes) {
    // filter fd
    if (fd < 2) return -1;
    // get inode from fd
    uint32_t inode = current_task_pcb->fd_arr[fd].inode;
    uint32_t file_pos = current_task_pcb->fd_arr[fd].file_position;
    int32_t ret;
    // handle when reading directory
    if (inode == 0) {
        if (file_pos >= b_block->num_dentries) return 0;
        ret = read_directory(file_pos, buf, nbytes);
        current_task_pcb->fd_arr[fd].file_position++;
        return ret;
    }
    // call read data with inode specified from fd
    ret = read_data(inode, file_pos, (uint8_t*)buf, (uint32_t)nbytes);
    // update file_pos
    current_task_pcb->fd_arr[fd].file_position += ret;
    return ret;
}
/*
file_write
Description: not allowed
Output: always -1
*/
int32_t file_write(int32_t fd, const void* buf, int32_t nbytes) {
    // read only fs, no writing allowed
    return -1;
}
/*
file_close
Description: does nothing
Output: always 0
*/
int32_t file_close(int32_t fd) {
    return 0;
}
