#include "rtc.h"
#include "../lib.h"
#include "../i8259.h"
#include "../tasks.h"

#define RTC_IRQ 0x08
#define RTC_PORT 0x70
#define RTC_DATA 0x71
#define SELECT_REG_A 0x8A
#define SELECT_REG_B 0x8B
#define SELECT_REG_C 0x8C
#define DISABLE_SET_AIE_UIE 0x4F
#define ENABLE_PIE 0x40
#define REG_A_MASK 0x80
#define ENABLE_OSCILLATOR 0x20
#define MAX_FREQ 32768
#define MIN_FREQ 2
#define RATE_START 15
#define RATE_END 3

#define rtc_called   rtc_called_virtual[0] \
				   = rtc_called_virtual[1] \
				   = rtc_called_virtual[2] \
				   = rtc_called_virtual[3] \
				   = rtc_called_virtual[4] \
				   = rtc_called_virtual[5] \
				   = rtc_called_virtual[6]

// each task can have its own virtual rtc
static int8_t rtc_opened_virtual[7];
static int8_t rtc_called_virtual[7];

int32_t* rtc_op_table[4] = {(int32_t*)rtc_open, (int32_t*)rtc_read, (int32_t*)rtc_write, (int32_t*)rtc_close};

/*
enable_rtc
Description: enables RTC IRQ 8 and sets frequency
Input: frequency
Output: none
Effect: enables RTC IRQ 8 and sets frequency
Source: osdev.org/RTC
*/
void enable_rtc(uint32_t frequency) {
	// prevent out of bound inputs
	if (frequency >= MAX_FREQ || frequency < MIN_FREQ) {
		printf("RTC not set, frequency out of range\n");
		return;
	}

	// calculate rate
	uint8_t rate = RATE_START;
	while (frequency > MIN_FREQ && rate >= RATE_END) {
		rate--;
		frequency/=2;
	}

	uint32_t flags;
	cli_and_save(flags); // disable interrupt

	uint8_t old, new;

	outb(SELECT_REG_A, RTC_PORT); // get reg A
	old = inb(RTC_DATA); // get old register A
	new = (old & REG_A_MASK) | (rate | ENABLE_OSCILLATOR); // enable oscillator and set rate
	outb(SELECT_REG_A, RTC_PORT);
	outb(new, RTC_DATA); // write rate

	restore_flags(flags);

	outb(SELECT_REG_B, RTC_PORT); // get reg B
	old = inb(RTC_DATA); // get old reg B
	new = (old & DISABLE_SET_AIE_UIE) | ENABLE_PIE; // mask to only enable PIE
	outb(SELECT_REG_B, RTC_PORT);
	outb(new, RTC_DATA); // enable int


	outb(SELECT_REG_C, RTC_PORT);	// read C to remove any flags
	inb(RTC_DATA);		// just throw away contents

	restore_flags(flags); // restore flags
}

/*
disable_rtc
Description: disables RTC IRQ 8
Input: none
Output: none
Effect: disables RTC IRQ 8
Source: osdev.org/RTC
*/
void disable_rtc() {
	uint32_t flags;
	cli_and_save(flags); // disable interrupt

	outb(SELECT_REG_B, RTC_PORT);
	uint8_t old = inb(RTC_DATA); // get old reg b
	outb(SELECT_REG_B, RTC_PORT);
	outb(old & ~(ENABLE_PIE) , RTC_DATA); // mask to disable PIE

	restore_flags(flags); // restore flags
}

/*
rtc_isr_handler
Description: handler for rtc interrupts
Input: none
Output: none
Effect: performs test_interrupts, increments interrupt counts
Source: osdev.org/RTC
*/
void rtc_isr_handler() {
	rtc_called = 1;
	/* clear Reg C by reading*/
	outb(0x0C, RTC_PORT);
	inb(RTC_DATA);
}

/*
* change_frequency
* DESCRIPTION: change the frequency of the RTC
* INPUTS: frequency - new frequency of RTC
* OUTPUTS: uint32_t - 0 if everything works, -1 otherwise
* SIDE EFFECTS: none
*/
uint32_t change_frequency(uint32_t frequency){
	// prevent out of bound inputs
	if (frequency >= MAX_FREQ || frequency < MIN_FREQ) {
		printf("RTC not set, frequency out of range\n");
		return -1;
	}

	// calculate rate
	uint8_t rate = RATE_START;
	while (frequency > MIN_FREQ && rate >= RATE_END) {
		rate--;
		frequency/=2;
	}

	uint32_t flags;
	cli_and_save(flags); // disable interrupt

	uint8_t old, new;

	outb(SELECT_REG_A, RTC_PORT); // get reg A
	old = inb(RTC_DATA); // get old register A
	new = (old & REG_A_MASK) | rate ; //set rate
	outb(SELECT_REG_A, RTC_PORT);
	outb(new, RTC_DATA); // write rate

	restore_flags(flags);

	return 0;
}

/*
* rtc_open
* DESCRIPTION: Initialize the RTC driver
* INPUTS: none
* OUTPUTS: int32_t - 0 if everything works
* SIDE EFFECTS: sets RTC frequency to 2 Hz and enables RTC and irq8
*/
int32_t rtc_open(int32_t fd, const uint8_t* filename) {
	rtc_opened_virtual[current_task_id] = 1;
	rtc_called_virtual[current_task_id] = 0;
	return 0;
}

/*
* rtc_close
* DESCRIPTION: Closes the RTC driver
* INPUTS: none
* OUTPUTS: int32_T - 0 because nothing happens
* SIDE EFFECTS: disable irq8, rtc
*/
int32_t rtc_close(int32_t fd){
	rtc_opened_virtual[current_task_id] = 0;
	return 0;
}

/*
* rtc_read
* DESCRIPTION: Block until the next RTC interrupt
* INPUTS: none used
* OUTPUTS: int32_t - 0 if everything works
* SIDE EFFECTS: none
*/
int32_t rtc_read(int32_t fd, void* buf, int32_t nbytes){
	while(!rtc_called_virtual[current_task_id]);
	rtc_called_virtual[current_task_id] = 0;
	return 0;
}

/*
* rtc_write
* DESCRIPTION: Change the frequency of the RTC
* INPUTS: buf - frequency to change to
* OUTPUTS: int32_t - 0 if everything works, -1 otherwise
* SIDE EFFECTS: none
*/
int32_t rtc_write(int32_t fd, const void* buf, int32_t nbytes){
	int32_t* freqs = (int32_t *) buf;
	int32_t change_result = change_frequency(freqs[0]);
	return change_result;
}
