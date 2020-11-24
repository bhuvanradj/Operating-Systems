.data
    USER_DS = 0x002B
    USER_CS = 0x0023
    PROGRAM_BOTTOM = 0x083FFFFC # end at ..FC since its 4 bytes from ..FF

syscall_return_val:
    .long 0

syscall_op_table:
    .long 0, halt, execute, read, write, open, close, getargs, vidmap

max_syscall:
    .long 8

.text

.globl system_call_entry
.globl start_program
.globl end_program

/*
system_call_entry
Description: Entry for system call at INT 0x80
Input: value in eax,ebx,ecx,edx
Output: return value in eax
*/
system_call_entry:
    pushal
    cld
    pushl %edx                      # pushed register arguements
    pushl %ecx
    pushl %ebx
    cmpl $0, %eax                   # filter eax to between 0 and 8
    je syscall_fail
    cmpl max_syscall, %eax
    ja syscall_fail
    sti
    call *syscall_op_table(,%eax,4) # use jump table
    cli                             # don't allow interrupts here, will mess up syscall_return_val
    syscall_done:
    movl %eax, syscall_return_val   # store return value since eax will be restored in popal
	addl $12, %esp                  # get rid of 3 arguments
	popal                           # restoring register values to hide register changes in system call
    movl syscall_return_val, %eax   # get return val back into eax
	iret
    syscall_fail:
    movl $-1, %eax
    jmp syscall_done

/*
start_program:
Description: stores return location of program, drops into user mode, jumps to first ins of program on iret
Input: entry_addr of program, exit_addr to return to previous program
Output: None

store the return location label in exit_addr
first block sets segment selectors to user data segment
the 3 lines after pushf forces iret to atomically to sti by modifying the interrupt flag
right after that we push the entry point for the program, when iret is called eip will be this entry point
SOURCE: http://www.jamesmolloy.co.uk/tutorial_html/10.-User%20Mode.html
*/

start_program:
    push %ebp
    mov %esp, %ebp
    push %ebx
    push %esi
    push %edi

    mov 8(%ebp), %ebx           # retrieve arguments
    mov 12(%ebp), %esi

    movl %ebp, (%esi)            # store ebp as the new tasks return ebp

    cli
    mov $USER_DS, %ax           # push USER_DS
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %fs
    mov %ax, %gs

    push $USER_DS               # push USER_DS so it loads in SS after IRET
    mov $PROGRAM_BOTTOM, %eax   # pointer to bottom of user program page
    push %eax                   # will act as the stack of the program, ESP after IRET
    pushf                       # Push flags
    pop %eax
    or $0x200, %eax             # set Interrupt Flag with 0x200 so STI on IRET
    push %eax
    push $USER_CS               # push USER_CS so it loads in CS after IRET
    push %ebx                   # new EIP after IRET
    iret

/*
end_program
Description: exit for each program
Input: status, previous tasks ebp to return to
Output: status in eax
*/
end_program:
    push %ebp
    mov %esp, %ebp
    mov 8(%ebp), %eax           # store status code in eax
    mov 12(%ebp), %ebx          # retrieve return ebp from arg
    mov %ebx, %ebp              # set ebp to previous tasks ebp
    mov %ebp, %esp
    sub $12, %esp               # retrieve the 3 callee saved regs
    pop %edi
    pop %esi
    pop %ebx
    leave                       # leave "as if" returning from that start_program
    ret
