#ifndef KB_H
#define KB_H

#include "../types.h"
#define KB_BUF_SIZE 128
/*
keyboard_isr_handler
Description: Gets scancode, converts to ascii, and prints out
Input: None
Output: None
Effect: prints scan and ascii code
Source: osdev.org/PS2
*/
extern void keyboard_isr_handler();

#endif
