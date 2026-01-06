#include <typedefs.h>
#include <cpu_scheduling/process.h>

extern tcb_t* current_task;
extern tcb_t* task_list_head;

extern void switch_task(tcb_t* old_task, tcb_t* next_task);

void schedule(){

    if(current_task == NULL || task_list_head == NULL){
        return;
    }

    tcb_t* old_task = current_task;
    tcb_t* next_task = current_task->next;

    if(next_task == NULL){
        next_task == task_list_head;
    }

    if(old_task == next_task){
        return;
    }

    current_task = next_task;

    switch_task(old_task, next_task);

}