.text

.globl	exception
.globl  interrupt
.globl  system_call_entry
.align   4

/*
array of the unique irq handlers
adds irq number as arguement
for handlers
*/
.align 4
exception:
   .long exception0
   .long exception1
   .long exception2
   .long exception3
   .long exception4
   .long exception5
   .long exception6
   .long exception7
   .long exception8
   .long exception9
   .long exception10
   .long exception11
   .long exception12
   .long exception13
   .long exception14
   .long exception15
   .long exception16
   .long exception17
   .long exception18
   .long exception19
   .long exception20
   .long exception21
   .long exception22
   .long exception23
   .long exception24
   .long exception25
   .long exception26
   .long exception27
   .long exception28
   .long exception29
   .long exception30
   .long exception31

.align 4
interrupt:
   .long interrupt0
   .long interrupt1
   .long interrupt2
   .long interrupt3
   .long interrupt4
   .long interrupt5
   .long interrupt6
   .long interrupt7
   .long interrupt8
   .long interrupt9
   .long interrupt10
   .long interrupt11
   .long interrupt12
   .long interrupt13
   .long interrupt14
   .long interrupt15


/*
# should push the correct irq number  #
*/

#define EXCEPTION(name,number)  \
.globl name                     ;\
name:                           ;\
    pushal                      ;\
    cld                         ;\
    pushl $number               ;\
    call exception_common       ;\
    addl $4, %esp               ;\
    popal                       ;\
    iret

EXCEPTION(exception0,0)
EXCEPTION(exception1,1)
EXCEPTION(exception2,2)
EXCEPTION(exception3,3)
EXCEPTION(exception4,4)
EXCEPTION(exception5,5)
EXCEPTION(exception6,6)
EXCEPTION(exception7,7)
EXCEPTION(exception8,8)
EXCEPTION(exception9,9)
EXCEPTION(exception10,10)
EXCEPTION(exception11,11)
EXCEPTION(exception12,12)
EXCEPTION(exception13,13)
EXCEPTION(exception14,14)
EXCEPTION(exception15,15)
EXCEPTION(exception16,16)
EXCEPTION(exception17,17)
EXCEPTION(exception18,18)
EXCEPTION(exception19,19)
EXCEPTION(exception20,20)
EXCEPTION(exception21,21)
EXCEPTION(exception22,22)
EXCEPTION(exception23,23)
EXCEPTION(exception24,24)
EXCEPTION(exception25,25)
EXCEPTION(exception26,26)
EXCEPTION(exception27,27)
EXCEPTION(exception28,28)
EXCEPTION(exception29,29)
EXCEPTION(exception30,30)
EXCEPTION(exception31,31)

###########################################################

#define INTERRUPT(name,number)  \
.globl name                     ;\
name:                           ;\
    pushal                      ;\
    cld                         ;\
    pushl $number               ;\
    call interrupt_common       ;\
    addl $4, %esp               ;\
    popal                       ;\
    iret

INTERRUPT(interrupt0,0)
INTERRUPT(interrupt1,1)
INTERRUPT(interrupt2,2)
INTERRUPT(interrupt3,3)
INTERRUPT(interrupt4,4)
INTERRUPT(interrupt5,5)
INTERRUPT(interrupt6,6)
INTERRUPT(interrupt7,7)
INTERRUPT(interrupt8,8)
INTERRUPT(interrupt9,9)
INTERRUPT(interrupt10,10)
INTERRUPT(interrupt11,11)
INTERRUPT(interrupt12,12)
INTERRUPT(interrupt13,13)
INTERRUPT(interrupt14,14)
INTERRUPT(interrupt15,15)
