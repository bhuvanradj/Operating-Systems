#ifndef SYSCALL_H
#define SYSCALL_H

#include "types.h"

/* Entry for system call */
extern int32_t system_call_entry();

extern int32_t halt (uint8_t status);
extern int32_t execute (const uint8_t* command);
extern int32_t read (int32_t fd, void* buf, int32_t nbytes);
extern int32_t write (int32_t fd, const void* buf, int32_t nbytes);
extern int32_t open (const uint8_t* filename);
extern int32_t close (int32_t fd);
extern int32_t getargs (uint8_t* buf, int32_t nbytes);
extern int32_t vidmap (uint8_t** screen_start);

int32_t start_program(uint32_t entry_addr, uint32_t* exit_ebp_ptr);
int32_t end_program(uint32_t exit_status, uint32_t exit_ebp);

#endif
