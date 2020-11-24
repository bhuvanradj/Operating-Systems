#ifndef PIT_H
#define PIT_H

#include "../types.h"
/*
* enable_pit
* DESCRIPTION: Programs the pit to get interrupts at approximately 10 ms
* INPUTS: none
* OUTPUTS: none
* SIDE EFFECTS: pit reload value is set, 10 ms pit interrupts
*/
extern void enable_pit();

#endif
