#include "syscall.h"

#include "lib.h"
#include "x86_desc.h"
#include "paging.h"
#include "tasks.h"
#include "scheduler.h"

#include "drivers/fs.h"
#include "drivers/term.h"
#include "drivers/rtc.h"

#define PROGRAM_IMAGE_VIRT_BASE 0x08000000
#define PROGRAM_IMAGE_PHYS_BASE 0x00800000
#define PROGRAM_IMAGE_SIZE      0x00400000
#define PROGRAM_IMAGE_OFFSET    0x00048000

/* halt
Description: ends currently executing program and return to previous program
Input: status
Output: will return to execute
*/
int32_t halt (uint8_t status){
    cli();
    int i;
    // close all files
    for (i = 2; i < 8; i++) {close(i);}
    // handle closing extra terminals
    if (current_task_pcb->parent_task_id == 0) {
        // find a terminal we can switch to
        int next_term_id;
        next_term_id = -1;
        for (i = 0; i < 3; i++) {
            if (i == current_task_pcb->terminal_id) continue;
            if (terms[i]._started) { next_term_id = i; break; }
        }
        // next term -1 means no other terms to switch to, will handle like regular program
        if (next_term_id != -1) {
            // set this terminal as not started
            terms[current_task_pcb->terminal_id]._started = 0;
            change_term(next_term_id);
            // remove task from task arr
            delete_task();
            // let scheduler move to next task without ever returning to this task
            scheduler_remove_shell();
        }
    }
    // pass exit ebp and status code
    end_program((uint32_t)status, get_pcb(current_task_pcb->parent_task_id)->exec_ebp);
    // should never get here
    return -1;
}

/* execute
Description: loads program image, sets up new paging for program,
adjust tss, moved instruction pointer to starting point of program
Input: arguements
Output: none
*/
int32_t execute (const uint8_t* command) {
    cli();
    int32_t i,j;
    int32_t exit_status;
    // add a new task, changes pcb
    if (new_task() == -1) {return -1;}
    // parse command, seperate program name from arguements
    i = 0;
    // strip leading spaces from args
    while(command[i] == ' ') i++;
    j = i;
    while(command[j] != ' ' && command[j] != 0 && j < 32 + i) {j++;}
    // copy program name
    uint8_t program_name[32];
    strncpy((int8_t*)program_name, (int8_t*)(&command[i]), j-i);
    program_name[j-i] = 0;

    // strip leading spaces from args
    i = j;
    while(command[i] == ' ') i++;
    // handle arguements
    for (current_task_pcb->args_length = 0; current_task_pcb->args_length < ARG_MAX_LENGTH; current_task_pcb->args_length++) {
        if (command[i+current_task_pcb->args_length] == '\0') break;
        else current_task_pcb->args[current_task_pcb->args_length] = command[i+current_task_pcb->args_length];
    }
    current_task_pcb->args[current_task_pcb->args_length] = '\0';
    // setup paging for the new program
    disable_all_pages();
    init_kernel_page();
    init_vidmem_pages();
    // map program image page
    map_4m_page(PROGRAM_IMAGE_PHYS_BASE + (PROGRAM_IMAGE_SIZE*(current_task_id-1)), PROGRAM_IMAGE_VIRT_BASE);
    reload_page_directory();

    // check if program exists
    if (-1 == file_open(2, program_name)) {exit_status = -1; goto EXIT;}
    // check if "program" is rtc or .
    if (current_task_pcb->fd_arr[2].inode == 0) {exit_status = -1; goto EXIT;}
    // load program image into new page, read all 4mb if can, file read will cut off by itself
    uint8_t* program_image_start = (uint8_t*) (PROGRAM_IMAGE_VIRT_BASE+PROGRAM_IMAGE_OFFSET);
    if (-1 == file_read(2, (void*)program_image_start, PROGRAM_IMAGE_SIZE)) {exit_status = -1; goto EXIT;}
    // check if not executable file, undo changes if not
    uint8_t executable[4] = {0x7f, 0x45, 0x4c, 0x46};
    for (i = 0; i < 4; i++) {
        if (program_image_start[i] != executable[i]) {
            exit_status = -1;
            goto EXIT;
        }
    }
    // update file directory, open stdin and stdout, close others
    set_fd(0, stdin_op_table, 0, 0, 1);
    set_fd(1, stdout_op_table, 0, 0, 1);
    term_open(0, (uint8_t*)"term");
    for (i = 2; i < 8; i++) {
        set_fd(i, 0, 0, 0, 0);
    }
    // set entry point as 32bit from bytes 24-27
    uint32_t entry_addr = *((uint32_t*)(program_image_start+24));
    // drop into user mode and start program
    if (new_term_flag != -1) {
        new_term_flag = -1;
        // don't allow exiting of terminal,
        // new terminals should not override exec_ebp of task 0, override so the ebp is saved to a useless location
        start_program(entry_addr, &(current_task_pcb->exec_ebp));
    }
    else {
        do {
            exit_status = start_program(entry_addr, &(get_pcb(current_task_pcb->parent_task_id)->exec_ebp));
        } while (current_task_pcb->parent_task_id == 0); // keep restarting if parent is 0 "kernel"
    }
    // returned from program, clean up task, and reload pd
    EXIT:
        delete_task();
        reload_page_directory();
        return exit_status;
}

/* read
Description: based on current program, use fd num to perform associated
"read" action based on file type at the fd num
Input: arguements
Output: none
*/
int32_t read(int32_t fd, void* buf, int32_t nbytes) {
    // check fd in bound
    if (fd < 0 || fd > 7) return -1;
    // return fail if file is not open
    if (current_task_pcb->fd_arr[fd].flags == 0) return -1;
    // get function pointer for read function
    int32_t (*func) (int32_t, void*, int32_t);
    func = (int32_t (*)(int32_t, void*, int32_t)) current_task_pcb->fd_arr[fd].file_op_table_ptr[FILE_OP_READ];
    // invoke function
    return func(fd, buf, nbytes);
}

/* write
Description: based on current program, use fd num to perform associated
"write" action based on file type at the fd num
Input: arguements
Output: none
*/
int32_t write (int32_t fd, const void* buf, int32_t nbytes) {
    // filter fd to between 0 and 7, (8 fd max)
    if (fd < 0 || fd > 7) return -1;
    // return fail if file is not open
    if (current_task_pcb->fd_arr[fd].flags == 0) return -1;
    // get pointer for write function
    int32_t (*func) (int32_t, const void*, int32_t);
    func = (int32_t (*)(int32_t, const void*, int32_t)) current_task_pcb->fd_arr[fd].file_op_table_ptr[FILE_OP_WRITE];
    // invoke function
    return func(fd, buf, nbytes);
}
/* open
Description: based on current program, use fd num to perform associated
"open" action based on file type at the fd num
Input: arguements
Output: fd of opened file,
*/
int32_t open (const uint8_t* filename) {
    // filter fd to between 0 and 7, (8 fd max)
    int32_t fd;
    for (fd = 2; fd < 9; fd++) {
        // no fd available
        if (fd == 8) return -1;
        // found available fd
        if ((current_task_pcb->fd_arr[fd]).flags == 0) break;
    }
    // check if opening rtc, if so set op table
    if (strncmp((int8_t*) filename, (int8_t*) "rtc", 3) == 0) {
        set_fd(fd, rtc_op_table, 0, 0, 0);
    }
    // else opening a fs file, set op table
    else {
        set_fd(fd, fs_op_table, 0, 0, 0);
    }
    // get pointer for open function
    int32_t (*func) (int32_t, const uint8_t*);
    func = (int32_t (*)(int32_t, const uint8_t*)) current_task_pcb->fd_arr[fd].file_op_table_ptr[FILE_OP_OPEN];
    // invoke function, set flag to present if successful, and return
    int32_t ret = func(fd, filename);
    if (ret == 0) {
        current_task_pcb->fd_arr[fd].flags = 1;
        return fd;
    }
    else return -1;

}

/* open
Description: based on current program, use fd num to perform associated
"open" action based on file type at the fd num
Input: arguements
Output: none
*/
int32_t close (int32_t fd) {
    // dont allow closing stdin stdout
    // filter fd to between 0 and 7, (8 fd max)
    if (fd < 2 || fd > 7) return -1;
    // return fail if file is not open
    if (current_task_pcb->fd_arr[fd].flags == 0) return -1;
    // get pointer for close function
    int32_t (*func) (int32_t);
    func = (int32_t (*)(int32_t)) current_task_pcb->fd_arr[fd].file_op_table_ptr[FILE_OP_CLOSE];
    // invoke function, clear flag if successful
    int32_t ret = func(fd);
    if (ret == 0) {(current_task_pcb->fd_arr[fd]).flags = 0;}
    return ret;
}

/* get args
Description: gets parameters passed in during execute
Input: buf to write to, number of bytes to write
Output: 0 on success, -1 on fail
*/
int32_t getargs (uint8_t* buf, int32_t nbytes) {
    // return -1 if no args, or args longer than nbytes
    if (current_task_pcb->args_length == 0) return -1;
    if (current_task_pcb->args_length > nbytes) return -1;
    // copy args
    strncpy((int8_t*)buf, (int8_t*) current_task_pcb->args, nbytes);
    return 0;
}

/* vidmap
Description: maps screen start to video mem
Input: screen_start
Output: 0 on success, -1 on fail
Effects: maps virtual addr screen_start to physical vmem
*/
int32_t vidmap (uint8_t** screen_start) {
    // check to see if permission level is 1 for this address
    if (check_permission((uint32_t)screen_start) < 1) return -1;
    // map video mem physical page to same location after the user program page
    map_4k_page(V_MEM_BASE, PROGRAM_IMAGE_VIRT_BASE+PROGRAM_IMAGE_SIZE+V_MEM_BASE);
    *screen_start = (uint8_t*) PROGRAM_IMAGE_VIRT_BASE+PROGRAM_IMAGE_SIZE+V_MEM_BASE;
    return 0;
}
