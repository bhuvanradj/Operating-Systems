#include "kb.h"
#include "../i8259.h"
#include "../lib.h"
#include "term.h"
#include "../tasks.h"

#define KB_PORT 0x60
#define ENTER 0x1C
#define BACKSPACE 0x0E
#define CTRL_1 0x1D
#define CTRL_0 0x9D
#define SHIFT_1_L 0x2A
#define SHIFT_0_L 0xAA
#define SHIFT_1_R 0x36
#define SHIFT_0_R 0xB6
#define CAPS_LOCK 0x3A
#define ALT_1 0x38
#define ALT_0 0xB8

#define ASCII_TABLE_SIZE 0x3A

#define KB_BUF_SIZE 128

static uint8_t scan_to_ascii[ASCII_TABLE_SIZE] =
{ 0x00, 0x1B, '1', '2', '3', '4', '5', '6',
'7', '8', '9', '0', '-', '=', 0x08, 0x09,
'q', 'w', 'e', 'r', 't', 'y', 'u' , 'i' ,
'o', 'p', '[', ']', 0x13,0x03,'a' ,  's',
'd', 'f', 'g', 'h', 'j', 'k', 'l' ,  ';',
'\'','`', 0x0F,'\\', 'z', 'x', 'c', 'v'  ,
'b', 'n', 'm', ',', '.', '/', 0x0F, 0x00,
0x00,' '
};

static uint8_t s_scan_to_ascii[ASCII_TABLE_SIZE] =
{ 0x00, 0x1B, '!', '@', '#', '$', '%', '^',
'&', '*', '(', ')', '_', '+', 0x08, 0x09,
'Q', 'W', 'E', 'R', 'T', 'Y', 'U' , 'I' ,
'O', 'P', '{', '}', 0x13,0x03,'A' ,  'S',
'D', 'F', 'G', 'H', 'J', 'K', 'L' ,  ':',
'\"','~', 0x0F,'|', 'Z', 'X', 'C', 'V'  ,
'B', 'N', 'M', '<', '>', '?', 0x0F, 0x00,
0x00,' '
};

uint8_t shift = 0;
uint8_t esc = 0;
uint8_t ctrl = 0;
uint8_t alt = 0;
uint8_t capslock = 0;

/*
* keyboard_isr_handler
* DESCRIPTION: Gets scancode of the key pressed and adds appropriate keys ot the buffer
* INPUT: None
* OUTPUT: None
* SIDE EFFECT: adds to buffer, clears buffer when enter is pressed
* Source: osdev.org/PS2
*/
void keyboard_isr_handler(void) {
    uint8_t scancode;
    scancode = inb(KB_PORT);

    // modifier keys leave automatically
    switch (scancode) {
        // set ctrl
        case CTRL_1: ctrl = 1; return;
        case CTRL_0: ctrl = 0; return;
        // set shift
        case SHIFT_1_L: shift++; return;
        case SHIFT_1_R: shift++; return;
        case SHIFT_0_L: if (shift > 0) shift--; return;
        case SHIFT_0_R: if (shift > 0) shift--; return;
        // set alt
        case ALT_1 : alt = !alt; return;
        case ALT_0 : alt = 0; return;
        // set capslock
        case CAPS_LOCK: capslock = !capslock; return;
    }
    // handle alt f1,f2,f3
    if (alt) {
        switch(scancode) {
            case 0x3B: change_term(0); break; // 0x3B = f1 scancode
            case 0x3C: change_term(1); break; // f2
            case 0x3D: change_term(2); break; // f3
        }
        return;
    }
    // handle if scancode is available
    if (scancode < ASCII_TABLE_SIZE) {
        // will need to write to active terminal, not the terminal for the task that running at this quantum
        rw_active_terminal = 1;
        // handle ctrl l
        if (ctrl && scan_to_ascii[scancode] == 'l') {
            // clear active terminal
            clear();
            puts((int8_t*)kb_buf);
        }
        // any other character
        else {
            if (kb_enabled){
                // handle enter
                if (scancode == ENTER) {
                    putc('\n');
                    kb_buf_ready = 1;
                }
                // handle backspace
                else if (scancode == BACKSPACE) {
                    if (kb_buf_index > 0) {
                        kb_buf_index--;
                        delc();
                    }
                }
                // any other character
                else if(kb_buf_index < KB_BUF_SIZE) {
                    kb_buf[kb_buf_index++] = (shift || capslock) ? s_scan_to_ascii[scancode] : scan_to_ascii[scancode];
                    putc(kb_buf[kb_buf_index-1]);
                }
            }
        }
        rw_active_terminal = 0;
    }
}
