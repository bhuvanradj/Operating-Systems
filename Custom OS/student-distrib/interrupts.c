#include "interrupts.h"

#include "lib.h"
#include "x86_desc.h"
#include "tasks.h"
#include "syscall.h"

#include "i8259.h"
#include "drivers/rtc.h"
#include "drivers/kb.h"
#include "scheduler.h"

/* exception_common
Description: Reports exception number
Input: irq number
Output: none
Effect: spin when exception occurs
*/
void exception_common(uint32_t irq) {
	printf("Exception %d: ", irq);
	switch (irq) {
		case 0: printf("DIV_BY_ZERO\n"); break;
		case 1: printf("DEBUG\n"); break;
		case 2: printf("NMI\n"); break;
		case 3: printf("BREAKPOINT\n"); break;
		case 4: printf("OVERFLOW\n"); break;
		case 5: printf("BOUND_RANGE_EXCEEDED\n"); break;
		case 6: printf("INVALID_OPCODE\n"); break;
		case 7: printf("DEVICE_NOT_AVAILABLE\n"); break;
		case 8: printf("DOUBLE_FAULT\n"); break;
		case 9: printf("SEGMENT_OVERRUN\n"); break;
		case 10: printf("INVALID_TSS\n"); break;
		case 11: printf("SEG_NOT_PRESENT\n"); break;
		case 12: printf("STACK_SEG_FAULT\n"); break;
		case 13: printf("GENERAL_PROTECTION_FAULT\n"); break;
		case 14: printf("PAGE_FAULT\n"); break;
		case 16: printf("FLOAT_EXCEPTION\n"); break;
		case 17: printf("ALIGNMENT_CHECK\n"); break;
		case 18: printf("MACHINE_CHECK\n"); break;
		case 19: printf("SIMD_FLOAT_EXCEPTION\n"); break;
		case 20: printf("FLOAT_EXCEPTION\n"); break;
		case 30: printf("SECURITY_EXCEPTION\n"); break;
        default: printf("Not an exception!\n"); break;
	}
	// exit to shell using status 256 to represent exception, as specified in shell
	// if shell somehow fails, it should just restart
	if (current_task_id > 0) {
		end_program(256, get_pcb(current_task_pcb->parent_task_id)->exec_ebp);
	}
	// if in kernel mode still, drop into infinite loop
	else {
		while(1);
	}
}

/* interrupt_common
Description: Calls IRQ handlers
Input: irq number
Output: none
Effect: sends eoi after handler completes
*/
void interrupt_common(uint32_t irq) {
	send_eoi(irq);
	switch (irq) {
		case 0: scheduler_isr_handler(); break;
		case 1: keyboard_isr_handler(); break;
		case 8: rtc_isr_handler(); break;
        default: printf("Interrupt %d cannot be handled!\n", irq); break;
	}
}

/* install_idt
Description: Installs handler in idt
Input: idt index, handler address
Output: none
Effect: Installs handler for specified irq
*/
void install_idt(uint8_t i, void* handler_addr) {
    if (handler_addr == 0) { printf("handler addr invalid\n"); return; }
	SET_IDT_ENTRY(idt[i], handler_addr); // set idt for div by 0
	idt[i].seg_selector = KERNEL_CS;
	idt[i].present = 1;
	idt[i].dpl = 0;
	if (i == 0x80) idt[i].dpl = 3;
	idt[i].size = 1;
	idt[i].reserved1 = idt[i].reserved2 = 1;
	idt[i].reserved0 = idt[i].reserved3 = idt[i].reserved4 = 0;
}
