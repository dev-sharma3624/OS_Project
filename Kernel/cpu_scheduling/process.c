#include <typedefs.h>
#include <cpu_scheduling/process.h>
#include <architecture/x86_64/trap_frame.h>
#include <memory_management/heap.h>
#include <memory_management/paging.h>

tcb_t* current_task = NULL;
tcb_t* task_list_head = NULL;

static uint64_t next_pid = 1;

extern void interrupt_return(void);

void multitask_init(){

    tcb_t* kernel_task = (tcb_t*) heap_kmalloc(sizeof(tcb_t));

    kernel_task->pid = next_pid++;
    kernel_task->next = kernel_task;
    kernel_task->task_state = TASK_RUNNING;

    current_task = kernel_task;
    task_list_head = kernel_task;

    current_task->pml4 = get_kernel_page_table(); 
    
    current_task->heap_end = 0x10000000;

}

void create_task(void (*entry_point) (void)){

    tcb_t* new_task = (tcb_t*) heap_kmalloc(sizeof(tcb_t));
    void* stack_bottom = heap_kmalloc(4096);

    uint64_t stack_top = (uint64_t) stack_bottom + 4096;

    stack_top -= sizeof(trap_frame_t);
    trap_frame_t* frame = (trap_frame_t*) stack_top;

    frame->rip = (uint64_t) entry_point;
    frame->cs = 0x08;
    frame->r_flags = 0x202;
    frame->rsp = stack_top;
    frame->ss = 0x10;

    frame->rax = 0; frame->rbx = 0; frame->rcx = 0;
    frame->rdx = 0; frame->rsi = 0; frame->rdi = 0;
    frame->rbp = 0; frame->r8 = 0;  frame->r9 = 0;
    frame->r10 = 0; frame->r11 = 0; frame->r12 = 0;
    frame->r13 = 0; frame->r14 = 0; frame->r15 = 0;
    frame->interrupt_no = 0;
    frame->error_code = 0;

    stack_top -= sizeof(uint64_t);
    *(uint64_t*) stack_top = (uint64_t) interrupt_return;

    stack_top -= sizeof(uint64_t) * 6;

    new_task->rsp = stack_top;
    new_task->pid = next_pid++;
    new_task->task_state = TASK_READY;

    tcb_t* temp = task_list_head;
    while (temp->next != task_list_head) {
        temp = temp->next;
    }
    new_task->next = task_list_head;
    temp->next = new_task;
    
}