#ifndef FS_H
#define FS_H

#include "../types.h"

#define BLOCK_SIZE 4096
#define NAME_LENGTH 32
#define MAX_OPENED_FILE 32

typedef struct dentry {
    uint8_t  f_name[NAME_LENGTH];
    uint32_t f_type;
    uint32_t inode_index;
    uint8_t  reserved[24]; // 24B Padding
} __attribute__((packed)) dentry_t;

typedef struct boot_block {
    uint32_t num_dentries;
    uint32_t num_inodes;
    uint32_t num_datablocks;
    uint8_t  reserved[52]; // 52B Padding
    dentry_t dentries[63]; // 63 dentry (each 64B) + boot block info (64B) should be 4096
} __attribute__((packed)) boot_block_t;

typedef struct inode {
    uint32_t length;
    uint32_t db_index[(BLOCK_SIZE/4)-1];
} __attribute__((packed)) inode_t;

typedef struct d_block {
    uint8_t data[BLOCK_SIZE];
} __attribute__((packed)) d_block_t;

extern int32_t filesystem_init(uint32_t module_start, uint32_t module_end);
extern int32_t read_dentry_by_name(const uint8_t * fname, dentry_t* dentry);
extern int32_t read_dentry_by_index(uint32_t index, dentry_t* dentry);
extern int32_t read_data(uint32_t inode, uint32_t offset, uint8_t* buf, uint32_t length);

extern int32_t* fs_op_table[4];

extern int32_t file_open(int32_t fd, const uint8_t* filename);
extern int32_t file_read(int32_t fd, void* buf, int32_t nbytes);
extern int32_t file_write(int32_t fd, const void* buf, int32_t nbytes);
extern int32_t file_close(int32_t fd);

#endif
