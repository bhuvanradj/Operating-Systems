/*
 * tuxctl-ioctl.c
 *
 * Driver (skeleton) for the mp2 tuxcontrollers for ECE391 at UIUC.
 *
 * Mark Murphy 2006
 * Andrew Ofisher 2007
 * Steve Lumetta 12-13 Sep 2009
 * Puskar Naha 2013
 */

#include <asm/current.h>
#include <asm/uaccess.h>

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/sched.h>
#include <linux/file.h>
#include <linux/miscdevice.h>
#include <linux/kdev_t.h>
#include <linux/tty.h>
#include <linux/spinlock.h>

#include "tuxctl-ld.h"
#include "tuxctl-ioctl.h"
#include "mtcp.h"

#define debug(str, ...) printk(KERN_DEBUG "%s: " str, __FUNCTION__, ## __VA_ARGS__)

#define SETNIBBLE 0xF
#define BITSIX 0X20
#define BITSEVEN 0X40
#define BTNMASK 0X9F
#define TEN 0X10
#define CLEAR 0X00



int tux_init(struct tty_struct* tty);

void tux_reset(struct tty_struct* tty);

int tux_buttons(struct tty_struct* tty, unsigned long arg);

int tux_set_LED(struct tty_struct* tty, unsigned long arg);

//int helper(int arg);

//unsigned long fract(unsigned long arg);

static int spam;

//static unsigned char butn[1];

//static unsigned long resethold;
 
static unsigned char buttoninfo[2];

/************************ Protocol Implementation *************************/

/* tuxctl_handle_packet()
 * IMPORTANT : Read the header for tuxctl_ldisc_data_callback() in
 * tuxctl-ld.c. It calls this function, so all warnings there apply
 * here as well.
 */
void tuxctl_handle_packet (struct tty_struct* tty, unsigned char* packet) {
    unsigned a, b, c;

    a = packet[0]; /* Avoid printk() sign extending the 8-bit */
    b = packet[1]; /* values when printing them. */
    c = packet[2];

    switch(a){
	case MTCP_ACK:
		spam=0;		//reset spam flag
		break;
	case MTCP_BIOC_EVENT:
		buttoninfo[0] = b;	//transfer button info to global var
		buttoninfo[1] = c;	//transfer button info to global var
		break;
	case MTCP_RESET:
		tux_reset(tty);		//call helper
		break;
	default:			
		return;
		
    }

    /*printk("packet : %x %x %x\n", a, b, c); */
}

int tuxctl_ioctl(struct tty_struct* tty, struct file* file,
                 unsigned cmd, unsigned long arg) {
/******** IMPORTANT NOTE: READ THIS BEFORE IMPLEMENTING THE IOCTLS ************
 *                                                                            *
 * The ioctls should not spend any time waiting for responses to the commands *
 * they send to the controller. The data is sent over the serial line at      *
 * 9600 BAUD. At this rate, a byte takes approximately 1 millisecond to       *
 * transmit; this means that there will be about 9 milliseconds between       *
 * the time you request that the low-level serial driver send the             *
 * 6-byte SET_LEDS packet and the time the 3-byte ACK packet finishes         *
 * arriving. This is far too long a time for a system call to take. The       *
 * ioctls should return immediately with success if their parameters are      *
 * valid.                                                                     *
 *                                                                            *
 ******************************************************************************/
    switch (cmd) {
        case TUX_INIT:
		spam = 0;	//reset spam flag
		tux_init(tty);	//call init helper
		return 0;	
        case TUX_BUTTONS:	
		if(arg==0) return -EINVAL;	//if arg is null ret error
		tux_buttons(tty, arg);		//call button helper
		return 0;	
        case TUX_SET_LED:
		if(spam!=0) return 0;		//if spam flag is active, return
		spam=1;				//else, set spam flag
		tux_set_LED(tty, arg);		//and also call LED helper
		return 0;
        default:
            return -EINVAL;			//return error if no cases are met
    }

}
/* 
 * tux_init
 * 	DESCRIPTION: initialization helper function for TUX
 * 	INPUTS:	tty -- controller struct
 * 	OUTPUTS: none
 * 	RETURN VALUE: none
 * 	SIDE EFFECTS: sends data to ldsic put functions
 */
int tux_init(struct tty_struct* tty){
	spam = 0;		//reset spam flag
	char msg[2];		//temp var to hold values we need to send
	msg[0] = MTCP_BIOC_ON;	//set BIOC ON
	msg[1] = MTCP_LED_USR;	//set LED USR
	tuxctl_ldisc_put(tty,msg,2);	//send data
	return 0;
}

/* 
 * tux_reset
 * 	DESCRIPTION: reset helper function for TUX
 * 	INPUTS:	tty -- controller struct
 * 	OUTPUTS: none
 * 	RETURN VALUE: none
 * 	SIDE EFFECTS: sends data to ldsic put functions
 */
void tux_reset(struct tty_struct* tty){
	char tmp[2];		//tmp var to hold values we need to send
	tmp[0] = MTCP_BIOC_ON;	//set BIOC ON
	tmp[1] = MTCP_LED_USR;	//set LED USR
	tuxctl_ldisc_put(tty,tmp,2);	//send data
	return;
}

/* 
 * tux_buttons
 * 	DESCRIPTION: button helper function for TUX
 * 	INPUTS:	tty -- controller struct
 * 		arg -- pointer to 32 bit int
 * 	OUTPUTS: none
 * 	RETURN VALUE: none
 * 	SIDE EFFECTS: copy's button info to user level
 */
int tux_buttons(struct tty_struct* tty, unsigned long arg){
	unsigned char RLDU;	//holds RLDU button bits
	unsigned char CBAS;	//holds CBAS button bits
	int binfo;		//holds data we want to send
	unsigned char tmp1,tmp2;	//tmp vars

	RLDU = buttoninfo[1];		//RLDU is given xxxxRDLU
	CBAS = buttoninfo[0];		//CBAS is given xxxxCBAS
	binfo = (CBAS & SETNIBBLE); 	//binfo get lower byte filled with CBAS
	binfo = binfo | ((RLDU & SETNIBBLE) << 4);	//binfo gets RLDUCBAS
	
	tmp1 = (binfo & BITSIX);	//only give bit six
	tmp2 = (binfo & BITSEVEN); 	//only give bit seven
	binfo = (binfo & BTNMASK); 	//AND with button mask

	tmp1 <<= 1;			//left shift once
	tmp2 >>= 1;			//right shift once
	binfo = binfo | tmp1;		//add to button info we want to send
	binfo = binfo | tmp2;		//add to button info we want to send

	copy_to_user((int*)arg, &binfo, sizeof(binfo));		//check for errors with copytouser
	return 0;
}

/* 
 * helper
 * 	DESCRIPTION: contains the proper LED HEX values based on which decimal value is passed in
 * 	INPUTS:	arg -- decimal value that is desired to be converted to TUX led val
 * 	OUTPUTS: none
 * 	RETURN VALUE: desired hex value
 * 	SIDE EFFECTS: none
 */
int helper(int arg){
	switch(arg){
		case(0):
			return 0xE7;
		case(1):
			return 0x06;
		case(2):
			return 0xCB;
		case(3):
			return 0x8F;
		case(4):
			return 0x2E;
		case(5):
			return 0xAD;
		case(6):
			return 0xED;
		case(7):
			return 0x86;
		case(8):
			return 0xEF;
		case(9):
			return 0xAE;
		case(10):
			return 0xEE;
		case(11):
			return 0x6D;
		case(12):
			return 0xE1;
		case(13):
			return 0x4F;
		case(14):
			return 0xE9;
		case(15):
			return 0xE8;
		default:
			return 0x0;

	}
}

/* 
 * tux_set_LED
 * 	DESCRIPTION: set LED helper function for TUX
 * 	INPUTS:	tty -- controller struct
 * 		arg -- LED info
 * 	OUTPUTS: LED display
 * 	RETURN VALUE: none
 * 	SIDE EFFECTS: controls LED display
 */
int tux_set_LED(struct tty_struct* tty, unsigned long arg){
	unsigned char ledbuf[6];	//bytes we want to send
	int fourmask = 0x000F;		//masks off everything except lower nibble
	int mask = 0x1;			//one bit mask
	int dots;			//dots on LED
	int numbers;			//number LED's
	int ledisp[4];			//tmp holder array for LED vals
	int tmp = arg;			//tmp holder for arg
	int i;

	ledbuf[0] = MTCP_LED_SET;	//set first byte in data packet
	ledbuf[1] = 0xF;		//set second byte in data packet


	dots = ((tmp >> 24) & SETNIBBLE);	//shift right and set with nibble of one's
	numbers = (SETNIBBLE & (tmp >> 16));	//shift left and set with nibble of one's
	
	for(i=0; i<4; i++){
		ledisp[i] = (fourmask & tmp);	//led holder array gets argument in array format
		tmp = tmp >> 4;
	}

	for(i=0; i<4; i++){
		int index = i+2;		//since first two packets are set, only touch the others
		if((numbers & mask)){		//if number mask and dot mask are on, call helper to set packet
			if(dots & mask) ledbuf[index] = helper(ledisp[i]) + TEN;  
			else ledbuf[index] = helper(ledisp[i]);	  //if dots is not set, give packet with adding ten
		}
		else ledbuf[index] = CLEAR;	//else give packet of zeroes
		numbers = numbers >> 1;		//right shift numbers
		dots = dots >> 1;		//right shift dots
						//continues to loop four times
	}

	tuxctl_ldisc_put(tty,ledbuf,i+2);	//send packet for LED's	
	return 0;
}


