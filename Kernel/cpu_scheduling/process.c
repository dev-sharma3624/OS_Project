#include <typedefs.h>
#include <cpu_scheduling/process.h>
#include <architecture/x86_64/trap_frame.h>
#include <memory_management/heap.h>
#include <memory_management/paging.h>
#include <memory_management/pmm.h>
#include <memory_management/memory.h>


#define USER_CS (0x20 | 3) 
#define USER_DS (0x28 | 3)

#define USER_CODE_VIRT  0x0000000000400000 // Standard ELF load address
#define USER_STACK_VIRT 0x00007FFFFFFFF000 // A high address in the lower half

extern uint64_t _KernelEnd;

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

    current_task->pml4 = (paging_page_table_t*) V2P((uint64_t)get_kernel_page_table()); 
    
    current_task->heap_end = get_heap_end_address();

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
    new_task->pml4 = (paging_page_table_t*) V2P((uint64_t)get_kernel_page_table());

    tcb_t* temp = task_list_head;
    while (temp->next != task_list_head) {
        temp = temp->next;
    }
    new_task->next = task_list_head;
    temp->next = new_task;
    
}

uint64_t* create_user_address_space() {

    uint64_t* new_pml4_phys = (uint64_t*)pmm_request_page();
    uint64_t* new_pml4_virt = (uint64_t*)P2V_DIRECT((uint64_t)new_pml4_phys);

    memset((void*) new_pml4_virt, 0, 4096);

    uint64_t* kernel_pml4_virt = (uint64_t*) get_kernel_page_table();
    for (int i = 256; i < 512; i++) {
        new_pml4_virt[i] = kernel_pml4_virt[i];
    }

    return new_pml4_phys; 
}

void create_user_task(uint64_t payload_phys_addr, size_t no_of_pages, paging_page_table_t* process_pml4_phys) {

    tcb_t* new_task = (tcb_t*) heap_kmalloc(sizeof(tcb_t));

    new_task->pml4 = process_pml4_phys;

    uint64_t u_stack_phys = (uint64_t) pmm_request_page();
    memset((void*) P2V_DIRECT(u_stack_phys), 0, 4096);
    paging_map_page(
        P2V_DIRECT(process_pml4_phys), 
        USER_STACK_VIRT, 
        u_stack_phys, 
        PT_FLAG_PRESENT | PT_FLAG_READ_WRITE | PT_FLAG_USER_SUPER, 
        KB_4
    );
    uint64_t u_stack_top = USER_STACK_VIRT + 4096;

    uint64_t* u_stack_phys_top = (uint64_t*)P2V_DIRECT(u_stack_phys + 4096 - 8);
    *u_stack_phys_top = 0;

    void* k_stack_bottom = heap_kmalloc(4096);
    uint64_t k_stack_top = (uint64_t)k_stack_bottom + 4096;

    k_stack_top -= sizeof(trap_frame_t);
    trap_frame_t* frame = (trap_frame_t*)k_stack_top;

    memset(frame, 0, sizeof(trap_frame_t));

    // --- THE RING 3 SETUP ---
    frame->rip = USER_CODE_VIRT;
    frame->cs  = USER_CS;     // Ring 3 Code
    frame->r_flags = 0x202;    // Interrupts Enabled
    frame->rsp = u_stack_top - 8; // Point to USER Stack!
    frame->ss  = USER_DS;     // Ring 3 Data

    k_stack_top -= sizeof(uint64_t);
    *(uint64_t*)k_stack_top = (uint64_t)interrupt_return;
    
    k_stack_top -= sizeof(uint64_t) * 6; 

    new_task->rsp = k_stack_top; // Scheduler switches KERNEL stacks
    new_task->pid = next_pid++;
    new_task->task_state = TASK_READY;
    new_task->stack_base = k_stack_bottom; // Remember for free()
    
    // Initialize Heap for sbrk (User standard: after kernel end)
    new_task->heap_end = USER_CODE_VIRT + (no_of_pages * 4096);

    tcb_t* temp = task_list_head;
    if (temp == NULL) {
        // Handle empty list case if necessary
        task_list_head = new_task;
        new_task->next = new_task;
    } else {
        while (temp->next != task_list_head) {
            temp = temp->next;
        }
        new_task->next = task_list_head;
        temp->next = new_task;
    }
}