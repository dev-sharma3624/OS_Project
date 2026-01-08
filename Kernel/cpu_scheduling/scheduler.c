#include <typedefs.h>
#include <cpu_scheduling/process.h>
#include <drivers/timer.h>

extern tcb_t* current_task;
extern tcb_t* task_list_head;

extern void switch_task(tcb_t* old_task, tcb_t* next_task);

void schedule(){

    if(current_task == NULL || task_list_head == NULL){
        return;
    }

    if (current_task->task_state == TASK_RUNNING) {
        current_task->task_state = TASK_READY;
    }

    tcb_t* old_task = current_task;
    tcb_t* next_task = current_task->next;

    if(next_task == NULL){
        next_task = task_list_head;
    }

    while(next_task->task_state != TASK_READY && next_task != current_task){
        next_task = next_task->next;
    }

    if(old_task == next_task){
        next_task->task_state = TASK_RUNNING;
        return;
    }

    current_task = next_task;
    next_task->task_state = TASK_RUNNING;

    
    switch_task(old_task, next_task);

}

void task_yield(){
    current_task->task_state = TASK_READY;
    schedule();
}



void task_sleep(uint64_t time_in_ms){
    current_task->wakeup_time = ticks + time_in_ms;
    current_task->task_state = TASK_BLOCKED;
    schedule();
}

void task_check_wakeup() {
    tcb_t* temp = task_list_head;
    
    if (temp == NULL) return;

    do {
        if (temp->task_state == TASK_BLOCKED && temp->wakeup_time <= ticks) {
            temp->task_state = TASK_READY;
        }
        temp = temp->next;
    } while (temp != task_list_head);
}