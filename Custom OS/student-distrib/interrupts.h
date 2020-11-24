#ifndef INTERRUPTS_H
#define INTERRUPTS_H

#include "types.h"

/* exception handler address pointers */
extern void* exception[32];
/* interrupt handler address pointers */
extern void* interrupt[16];

/* exception_common
Description: Reports exception number
Input: irq number
Output: none
Effect: spin when exception occurs
*/
extern void exception_common(uint32_t irq);

/* interrupt_common
Description: Calls IRQ handlers
Input: irq number
Output: none
Effect: sends eoi after handler completes
*/
extern void interrupt_common(uint32_t irq);

/* install_idt
Description: Installs handler in idt
Input: idt index, handler address
Output: none
Effect: Installs handler for specified irq
*/
extern void install_idt(uint8_t i, void* handler_addr);


#endif
