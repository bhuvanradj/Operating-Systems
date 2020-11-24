#ifndef TERM_H
#define TERM_H

#include "../types.h"

extern int32_t* stdin_op_table[4];
extern int32_t* stdout_op_table[4];

extern int32_t new_term_flag;

typedef struct term {
    // display info
    char* _mem_addr;
    int32_t _screen_x;
    int32_t _screen_y;
    // keyboard info
    uint8_t _kb_enabled;
    uint8_t _kb_buf[128];
    uint8_t _kb_buf_index;
    uint8_t _kb_buf_ready;

    // keep track of if terminal is started
    int32_t _started;
} __attribute__((packed)) term_t;

extern term_t terms[3];
// keep track of active terminal
int32_t active_terminal_id;
// flag to allow to write to 1) active terminal 0) terminal for this task
int32_t rw_active_terminal;

// define these "variables" to the terminal variable for the current task, or active terminal
#define video_mem       (terms[(rw_active_terminal) ? active_terminal_id : current_task_pcb->terminal_id]._mem_addr)
#define screen_x        (terms[(rw_active_terminal) ? active_terminal_id : current_task_pcb->terminal_id]._screen_x)
#define screen_y        (terms[(rw_active_terminal) ? active_terminal_id : current_task_pcb->terminal_id]._screen_y)
#define kb_enabled      (terms[(rw_active_terminal) ? active_terminal_id : current_task_pcb->terminal_id]._kb_enabled)
#define kb_buf          (terms[(rw_active_terminal) ? active_terminal_id : current_task_pcb->terminal_id]._kb_buf)
#define kb_buf_index    (terms[(rw_active_terminal) ? active_terminal_id : current_task_pcb->terminal_id]._kb_buf_index)
#define kb_buf_ready    (terms[(rw_active_terminal) ? active_terminal_id : current_task_pcb->terminal_id]._kb_buf_ready)

/*
change_term
Description: changes terminal, remaps physical address for any vidmap
if shell is not running on that terminal, will start it up automatically
Input: terminal to change to
Output: none
*/
extern void change_term(int n);
/*
* term_open
* DESCRIPTION: clears kb buffer and enables kb
* INPUT: None
* OUTPUT: 0 on success
*/
extern int32_t term_open(int32_t fd, uint8_t* filename);

/*
* term_read
* DESCRIPTION: once enter is pressed, copy nbytes of kb_buf into buf
* INPUT: buf to copy to , nbytes to copy
* OUTPUT: 0 on success
*/
extern int32_t term_read(int32_t fd, void* buf, int32_t nbytes);

/*
* term_write
* DESCRIPTION: print nbytes from buf to display
* INPUT: buf to print, nbytes to print
* OUTPUT: 0 on success
*/
extern int32_t term_write(int32_t fd, const void* buf, int32_t nbytes);

/*
* term_close
* DESCRIPTION: disables kb
* INPUT: none
* OUTPUT: 0 on success
*/
extern int32_t term_close();

#endif
