#ifndef TASKS_H
#define TASKS_H

#include "types.h"

#define MAX_TASK 7
#define ARG_MAX_LENGTH 128
#define FILE_OP_OPEN 0
#define FILE_OP_READ 1
#define FILE_OP_WRITE 2
#define FILE_OP_CLOSE 3

typedef struct file_desc {
    int32_t** file_op_table_ptr;
    uint32_t inode;
    uint32_t file_position;
    uint32_t flags; // 1 if open, 0 if closed
} __attribute__((packed)) file_desc_t;

typedef struct pcb {
    file_desc_t fd_arr[8]; // file descriptors

    uint32_t parent_task_id; // id of parent task to return to
    uint32_t sched_ebp; // ebp for scheduler to save/return to
    uint32_t exec_ebp;  // ebp for execute/halt to return to

    uint32_t terminal_id; // terminal this task is running under

    uint8_t args[ARG_MAX_LENGTH]; // args 
    uint8_t args_length;
} __attribute__((packed)) pcb_t;

extern int32_t current_task_id;
extern pcb_t*  current_task_pcb;
extern int32_t num_open_tasks;

extern int32_t new_task();
extern int32_t delete_task();
extern int32_t change_task(uint32_t task_num);

extern pcb_t* get_pcb(int32_t task_num);

extern int32_t set_fd(int32_t fd, int32_t** file_op_table_ptr, uint32_t inode, uint32_t file_position, uint32_t flags);

#endif
