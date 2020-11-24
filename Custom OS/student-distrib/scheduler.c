#include "scheduler.h"
#include "tasks.h"
#include "syscall.h"
#include "lib.h"
#include "paging.h"

#define TASK_QUEUE_LENGTH 15

int32_t task_queue[TASK_QUEUE_LENGTH];
int32_t head = 0;
int32_t tail = 0;

/*
scheduler_isr_handler
Description: moves on to next task in scheudler queue on PIT interrupt,
will leave this function as the next task
Input: none
Output: none
*/
void scheduler_isr_handler() {
    // don't bother if there is only 1 task running.
    if (head == tail) return;
    // critical section since we'll be changing task_queue
    cli();
    // get next task from front of queue
    int32_t this_task = current_task_id;
    int32_t next_task = task_queue[head];
    head = (head + 1) % TASK_QUEUE_LENGTH;
    // put currently running task at back of queue
    task_queue[tail] = current_task_id;
    tail = (tail + 1) % TASK_QUEUE_LENGTH;
    scheduler_next_ASM(&(current_task_pcb->sched_ebp), get_pcb(next_task)->sched_ebp);
    // perform task switch back to this_task
    change_task(this_task);
    reload_page_directory();
}

/*
scheduler_add_shell
Description: adds the current task to the queue, the "current" will leave this function
after scheduler_isr_handler changes back to "this task"
Input: none
Output: none
*/
void scheduler_add_shell() {
    int32_t this_task = current_task_id;
    // critical section since we'll be changing task_queue
    cli();
    // add currently operating task to the end of task queue
    task_queue[tail] = current_task_id;
    tail = (tail + 1) % TASK_QUEUE_LENGTH;
    scheduler_execute_ASM(&(current_task_pcb->sched_ebp));
    // previous function will return here after scheduler
    // switches into task that orignally called this function
    change_task(this_task);
    reload_page_directory();
}

/*
scheduler_remove_shell
Description: moves on to next task in the queue without adding current task back, effectively
deleting the current task from the queue
Input: none
Output: none
*/
void scheduler_remove_shell() {
    int32_t next_task = task_queue[head];
    head = (head + 1) % TASK_QUEUE_LENGTH;
    // move on to next task without adding current task
    scheduler_next_ASM(&(current_task_pcb->sched_ebp), get_pcb(next_task)->sched_ebp);
}

/* shell_caller
Description: Helper to call execute shell. 
*/
void shell_caller() {
    execute((uint8_t*)"shell");
}
