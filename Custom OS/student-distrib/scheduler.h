#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "types.h"

extern void scheduler_isr_handler();

extern void scheduler_add_shell();

extern void scheduler_remove_shell();

void scheduler_next_ASM(uint32_t* this_ebp, uint32_t next_ebp);

void scheduler_execute_ASM(uint32_t* this_ebp);

#endif
