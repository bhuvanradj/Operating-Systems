#ifndef RTC_H
#define RTC_H

#include "../types.h"

extern int32_t* rtc_op_table[4];

void enable_rtc(uint32_t frequency);
/*
rtc_isr_handler
Description: handler for rtc interrupts
Input: none
Output: none
Effect: performs test_interrupts, increments interrupt counts
Source: osdev.org/RTC
*/
extern void rtc_isr_handler();

/*
* rtc_open
* DESCRIPTION: Initialize the RTC driver
* INPUTS: none
* OUTPUTS: int32_t - 0 if everything works
* SIDE EFFECTS: sets RTC frequency to 2 Hz and enables RTC and irq8
*/
extern int32_t rtc_open(int32_t fd, const uint8_t* filename);

/*
* rtc_close
* DESCRIPTION: Closes the RTC driver
* INPUTS: none
* OUTPUTS: int32_T - 0 because nothing happens
* SIDE EFFECTS: disable irq8, rtc
*/
extern int32_t rtc_close(int32_t fd);

/*
* rtc_read
* DESCRIPTION: Block until the next RTC interrupt
* INPUTS: none used
* OUTPUTS: int32_t - 0 if everything works
* SIDE EFFECTS: none
*/
extern int32_t rtc_read(int32_t fd, void* buf, int32_t nbytes);

/*
* rtc_write
* DESCRIPTION: Change the frequency of the RTC
* INPUTS: buf - frequency to change to
* OUTPUTS: int32_t - 0 if everything works, -1 otherwise
* SIDE EFFECTS: none
*/
extern int32_t rtc_write(int32_t fd, const void* buf, int32_t nbytes);

#endif
