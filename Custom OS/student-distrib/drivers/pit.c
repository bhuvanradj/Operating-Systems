#include "pit.h"
#include "../lib.h"
#include "../i8259.h"

#define PIT_IRQ 0x00
#define PIT_INIT_PORT 0x43
#define PIT_DATA 0x40
#define RELOAD_VALUE 11931
#define PIT_INIT_WORD 0x34
/*
* enable_pit
* DESCRIPTION: Programs the pit to get interrupts at approximately 10 ms
* INPUTS: none
* OUTPUTS: none
* SIDE EFFECTS: pit reload value is set, 10 ms pit interrupts
*/

void enable_pit() {
    /*source:
    https://wiki.osdev.org/Programmable_Interval_Timer*/
    asm volatile("pushf            \n\
                  cli               \n\
                  movl %0, %%eax     \n\
                  outb %%al, $0x43  \n\
                  movl %1, %%eax     \n\
                  outb %%al, $0x40  \n\
                  movb %%ah, %%al   \n\
                  outb %%al, $0x40  \n\
                  sti               \n\
                  popf             \n\
                  "
                  :
                  :"r" (PIT_INIT_WORD), "r" (RELOAD_VALUE)
                  );
}
