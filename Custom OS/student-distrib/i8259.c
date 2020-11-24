/* i8259.c - Functions to interact with the 8259 interrupt controller
 * vim:ts=4 noexpandtab
 */

#include "i8259.h"
#include "lib.h"
#define EOI_COM 0x20
/* Interrupt masks to determine which interrupts are enabled and disabled */
// uint8_t master_mask; /* IRQs 0-7  */
// uint8_t slave_mask;  /* IRQs 8-15 */
uint16_t irq_mask = 0xffff;
#define master_mask	(irq_mask)
#define slave_mask	(irq_mask >> 8)

/* Initialize the 8259 PIC */
/* From linux/irq-i8259.c */
void i8259_init(void) {

	printf("Initializing 8259...\n");

	uint32_t flags;
	cli_and_save(flags); // disable interrupt

    outb(0xff, MASTER_8259_PORT_DATA);	/* mask all of 8259A-1 */
    outb(0xff, SLAVE_8259_PORT_DATA);	/* mask all of 8259A-2 */

    outb(ICW1, MASTER_8259_PORT);	/* ICW1: select 8259A-1 init */
    outb(ICW2_MASTER, MASTER_8259_PORT_DATA);	/* ICW2: 8259A-1 IR0 mapped to I8259A_IRQ_BASE + 0x00 */
    outb(ICW3_MASTER, MASTER_8259_PORT_DATA);	/* 8259A-1 (the master) has a slave on IR2 */
    outb(ICW4, MASTER_8259_PORT_DATA);

    outb(ICW1, SLAVE_8259_PORT);	/* ICW1: select 8259A-2 init */
    outb(ICW2_SLAVE, SLAVE_8259_PORT_DATA);	/* ICW2: 8259A-2 IR0 mapped to I8259A_IRQ_BASE + 0x08 */
    outb(ICW3_SLAVE, SLAVE_8259_PORT_DATA);	/* 8259A-2 is a slave on master's IR2 */
    outb(ICW4, SLAVE_8259_PORT_DATA); /* (slave's support for AEOI in flat mode is to be investigated) */

    outb(master_mask, MASTER_8259_PORT_DATA); /* restore master IRQ mask */
    outb(slave_mask, SLAVE_8259_PORT_DATA);	  /* restore slave IRQ mask */

	/* Enable Slave reporting on Master Line 2 */
	enable_irq(2);

	restore_flags(flags); // restore flags
}

/* Enable (unmask) the specified IRQ */
/* From linux/irq-i8259.c */
void enable_irq(uint32_t irq_num) {

	if (irq_num >= 16) { printf("IRQ out of bound\n"); return; }

    uint16_t mask;
	// printf("Enabling IRQ on Line %d\n", irq_num);

	uint32_t flags;
	cli_and_save(flags); // disable interrupt

	/* Create new mask and send to 8259 */
	mask = ~(1 << irq_num);
	irq_mask &= mask;
	if (irq_num & 8)
		outb(slave_mask, SLAVE_8259_PORT_DATA);
	else
		outb(master_mask, MASTER_8259_PORT_DATA);

	restore_flags(flags); // restore flags
}

/* Disable (mask) the specified IRQ */
/* From linux/irq-i8259.c*/
void disable_irq(uint32_t irq_num) {

	if (irq_num >= 16) { printf("IRQ out of bound\n"); return; }

    uint16_t mask;
	// printf("Disabling IRQ on Line %d\n", irq_num);

	uint32_t flags;
	cli_and_save(flags); // disable interrupt

	/* Create new mask and send to 8259 */
    mask = 1 << irq_num;
	irq_mask |= mask;
	if (irq_num & 8)
		outb(slave_mask, SLAVE_8259_PORT_DATA);
	else
		outb(master_mask, MASTER_8259_PORT_DATA);

	restore_flags(flags); // restore flags
}

/* Send end-of-interrupt signal for the specified IRQ */
/* Source: osdev.org/8259_PIC */
void send_eoi(uint32_t irq_num) {

	if (irq_num >= 16) { printf("IRQ out of bound\n"); return; }

	/* Intel Manual: "EOI must be issued twice in cascaded mode
    to slave and master" */
    if (irq_num & 8) {
        outb(EOI_COM, SLAVE_8259_PORT);
	}
    outb(EOI_COM, MASTER_8259_PORT);
}
