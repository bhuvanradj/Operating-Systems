.text

.globl scheduler_next_ASM
.globl scheduler_execute_ASM

/*
scheduler_isr_handler
Description: saves ebp for current task, retrieves ebp for next task,
leaves this function using ebp of the next task
Input: uint32_t* this_ebp, uint32_t next_ebp
Output: none
*/
scheduler_next_ASM:
    push %ebp
    mov  %esp, %ebp             # ebp = esp
    mov  8(%ebp), %ecx          # current_task_pcb->ebp = ebp
    mov  %ebp, (%ecx)
    mov  12(%ebp), %ebp         # ebp = next_pcb->ebp
    mov  %ebp, %esp             # esp = ebp
    leave                       # return within the next task
    ret

/*
scheduler_isr_handler
Description: sets up stack, and saves ebp in the same format as scheduler isr handler,
then calls shell, will never return
Input: uint32_t* this_ebp
Output: none
*/
scheduler_execute_ASM:
    push %ebp
    mov  %esp, %ebp
    mov  8(%ebp), %ecx          # current_task_pcb->ebp = ebp
    mov  %ebp, (%ecx)
    call shell_caller           # call execute("shell")
