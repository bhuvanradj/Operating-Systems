#include "tasks.h"
#include "types.h"
#include "x86_desc.h"
#include "drivers/term.h"

#define EIGHT_KB 0x00002000
#define KERNEL_TOP 0x00400000
#define KERNEL_BOTTOM 0x00800000

// task 0 will be occupied by the kernel itself
int32_t current_task_id = 0;
pcb_t* current_task_pcb = (pcb_t*)(KERNEL_BOTTOM - EIGHT_KB + 1);

int32_t task_arr[MAX_TASK] = {1,0,0,0,0,0,0};
int32_t num_open_tasks = 1;

/* new_task
Description: allocated task id for new tasks, changes into that task
Input: none
Output: new current task
*/
int32_t new_task() {
    int task_num;
    for (task_num = 1; task_num < MAX_TASK; task_num++) {
        // empty task
        if (!task_arr[task_num]) break;
    }
    // allow up to max task
    if (task_num >= MAX_TASK) return -1;
    // mark this task as being used
    task_arr[task_num] = 1;
    num_open_tasks++;
    int parent_id = current_task_id;
    change_task(task_num);
    // if new terminal, set parent task as kernel
    if (new_term_flag != -1) {
        current_task_pcb->parent_task_id = 0;
        current_task_pcb->terminal_id = new_term_flag;
    }
    // else, set parent task to previous task, and set terminal to parent's terminal
    else {
        current_task_pcb->parent_task_id = parent_id;
        current_task_pcb->terminal_id = get_pcb(parent_id)->terminal_id;
    }
    return current_task_id;
}

/* decrement_task
Description: removes current task and return to parent task
Input: none
Output: current task (parent)
*/
int32_t delete_task() {
    // free up this task
    task_arr[current_task_id] = 0;
    num_open_tasks--;
    // change to its parent
    change_task(current_task_pcb->parent_task_id);
    return current_task_id;
}

/* change_task
Description: changes esp and current pcb to current task
Input: none
Output: 0 if successful, -1 if fail
*/
int32_t change_task(uint32_t task_num) {
    current_task_id = task_num;
    current_task_pcb = (pcb_t*) (KERNEL_BOTTOM - (task_num * EIGHT_KB) - EIGHT_KB + 1);
    tss.esp0 = KERNEL_BOTTOM - (task_num * EIGHT_KB);
    return 0;
}

/* set_fd
Description: set the parameters of the current pcb
Input: none
Output: 0 if successful, -1 if fail
*/
int32_t set_fd(int32_t fd, int32_t** file_op_table_ptr, uint32_t inode, uint32_t file_position, uint32_t flags) {
    // filter fd to between 0 and 7, (8 fd max)
    if (fd < 0 || fd > 7) return -1;
    (current_task_pcb->fd_arr[fd]).file_op_table_ptr = file_op_table_ptr;
    (current_task_pcb->fd_arr[fd]).inode = inode;
    (current_task_pcb->fd_arr[fd]).file_position = file_position;
    (current_task_pcb->fd_arr[fd]).flags = flags;
    return 0;
}

pcb_t* get_pcb(int32_t task_num) {
    return (pcb_t*) (KERNEL_BOTTOM - (task_num * EIGHT_KB) - EIGHT_KB + 1);
}
