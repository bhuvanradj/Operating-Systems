#include "term.h"
#include "kb.h"
#include "../lib.h"
#include "../types.h"
#include "../paging.h"
#include "../tasks.h"
#include "../scheduler.h"

#define LINE_FEED 0xA
#define SCREEN_PAGE_SIZE 4096

int32_t term_unavail();

int32_t* stdin_op_table[4] = {(int32_t*)term_open, (int32_t*)term_read, (int32_t*)term_unavail, (int32_t*)term_close};
int32_t* stdout_op_table[4] = {(int32_t*)term_open, (int32_t*)term_unavail, (int32_t*)term_write, (int32_t*)term_close};

// backbuffer for each terminal
const uint32_t term_buf_addr[3] = {V_MEM_TERM_1,V_MEM_TERM_2,V_MEM_TERM_3};

// 3 terminals initialize statically
term_t  terms[3] = {{(char*)V_MEM_BASE, 0, 0, 0, {0}, 0, 0, 1},{(char*)V_MEM_TERM_2, 0, 0, 0, {0}, 0, 0, 0},{(char*)V_MEM_TERM_3, 0, 0, 0, {0}, 0, 0, 0}};
// keep track of active terminal
int32_t active_terminal_id = 0;
// flag to force reading and writing of terminal variables to 1) active terminal 0) terminal for currently running task
int32_t rw_active_terminal = 0;
// let tasking know to set correct terminal if a new terminal needs to be started, -1 means no, 0,1,2 means new terms on 0,1,2
int32_t new_term_flag = -1;

/*
change_term
Description: changes terminal, remaps physical address for any vidmap
if shell is not running on that terminal, will start it up automatically
Input: terminal to change to
Output: none
*/
void change_term(int next_term_id) {
    if (next_term_id < 0 || next_term_id > 2) return;
    // do nothing if this is active terminal
    if (active_terminal_id == next_term_id) return;

    if (!terms[next_term_id]._started && num_open_tasks == MAX_TASK) {
        printf("Too many processes running! Cannot open another terminal!\n");
        return;
    }

    // set address of terminal leaving to backbuffer
    terms[active_terminal_id]._mem_addr = (char*) term_buf_addr[active_terminal_id];
    // set address for terminal changing into as V_MEM
    terms[next_term_id]._mem_addr = (char*)V_MEM_BASE;

    // copy V_MEM to backbuffer
    memcpy((char*)term_buf_addr[active_terminal_id], (char*)V_MEM_BASE, SCREEN_PAGE_SIZE);
    // copy next backbuffer to vmem
    memcpy((char*)V_MEM_BASE, (char*)term_buf_addr[next_term_id], SCREEN_PAGE_SIZE);

    // remap vidmap, 0x0840000+V_MEM_BASE is the virtual address for vidmap pages
    remap_terminal_vidmap(active_terminal_id, term_buf_addr[active_terminal_id], 0x08400000+V_MEM_BASE);
    remap_terminal_vidmap(next_term_id, V_MEM_BASE, 0x08400000+V_MEM_BASE);

    // set next as active terminal
    active_terminal_id = next_term_id;
    rw_active_terminal = 1;
    move_cursor();
    rw_active_terminal = 0;

    // if shell for this term has not been executed, execute it through the scheduler
    if (!terms[next_term_id]._started) {
        // clear this screen on new terminal
        rw_active_terminal = 1;
        clear();
        rw_active_terminal = 0;
        // execute new shell for this new terminal
        terms[next_term_id]._started = 1;
        new_term_flag = next_term_id;
        scheduler_add_shell();
    }
}
/*
* term_open
* DESCRIPTION: clears kb buffer and enables kb
* INPUT: None
* OUTPUT: 0 on success
*/
int32_t term_open(int32_t fd, uint8_t* filename) {
    // clear buffer on open
    memset(kb_buf, 0 ,KB_BUF_SIZE);
    kb_buf_index = 0;
    kb_buf_ready = 0;
    kb_enabled = 0;
    return 0;
}

/*
* term_read
* DESCRIPTION: once enter is pressed, copy nbytes of kb_buf into buf
* INPUT: buf to copy to , nbytes to copy
* OUTPUT: 0 on success
*/
int32_t term_read(int32_t fd, void* buf, int32_t nbytes) {
    int i;
    // wipe buf
    memset(buf, 0, nbytes);
    kb_enabled = 1;
    // spin while waiting for buffer to be ready
    while(!kb_buf_ready);
    // buffer ready, disable writing to buffer
    kb_enabled = 0;
    for (i = 0; i < nbytes && i < kb_buf_index; i++) {
        ((uint8_t*)buf)[i] = kb_buf[i];
    }
    // append line feed character
    if (i < nbytes) {
        ((uint8_t*)buf)[i] = LINE_FEED;
    }
    // clear buffer
    memset(kb_buf,0,KB_BUF_SIZE);
    kb_buf_index = 0;
    kb_buf_ready = 0;
    return i+1;
}

/*
* term_write
* DESCRIPTION: print nbytes from buf to display
* INPUT: buf to print, nbytes to print
* OUTPUT: 0 on success
*/
int32_t term_write(int32_t fd, const void* buf, int32_t nbytes) {
    int i;
    for(i = 0; i < nbytes; i++) {
        putc(((int8_t*)buf)[i]);
    }
    return 0;
}

/*
* term_close
* DESCRIPTION: disables kb
* INPUT: none
* OUTPUT: 0 on success
*/
int32_t term_close() {
    kb_enabled = 0;
    return 0;
}

int32_t term_unavail() {
    return -1;
}
