#include <typedefs.h>
#include <cpu_scheduling/process.h>
#include <architecture/x86_64/trap_frame.h>
#include <memory_management/heap.h>
#include <memory_management/paging.h>
#include <memory_management/pmm.h>
#include <memory_management/memory.h>


#define USER_CS (0x20 | 3) 
#define USER_DS (0x28 | 3)

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

void create_user_task(void (*entry_point)(void)) {
    // 1. Allocate TCB
    tcb_t* new_task = (tcb_t*) heap_kmalloc(sizeof(tcb_t));
    
    // 2. Allocate KERNEL Stack (Used when syscalls/interrupts happen)
    void* k_stack_bottom = heap_kmalloc(4096);
    uint64_t k_stack_top = (uint64_t)k_stack_bottom + 4096;

    // 3. Allocate USER Stack (Used by the shell/app)
    // In a real OS, we'd map a page at a high address. 
    // For now, we alloc a frame and use its physical address (Identity Map cheat).
    uint64_t u_stack_phys = (uint64_t) pmm_request_page();

    paging_map_page(
        get_kernel_page_table(), 
        (uint64_t) u_stack_phys, // Virt
        (uint64_t) u_stack_phys, // Phys
        PT_FLAG_PRESENT | PT_FLAG_READ_WRITE | PT_FLAG_USER_SUPER // <--- ALLOW USER
    );

    void* u_stack_bottom = P2V(u_stack_phys);
    uint64_t u_stack_top = (uint64_t)u_stack_bottom + 4096;

    uint64_t code_phys = V2P((uint64_t)entry_point);
    uint64_t code_virt = (uint64_t)entry_point; // Use the same address for simplicity

    // Force map the code page as USER accessible
    paging_map_page(
        get_kernel_page_table(), 
        code_virt,
        code_phys,
        PT_FLAG_PRESENT | PT_FLAG_READ_WRITE | PT_FLAG_USER_SUPER // <--- ALLOW USER
    );

    // 4. Setup the Trap Frame on the KERNEL Stack
    k_stack_top -= sizeof(trap_frame_t);
    trap_frame_t* frame = (trap_frame_t*)k_stack_top;

    // Zero out the frame to avoid garbage values in registers
    // (Assuming you have memset, otherwise do it manually)
    memset(frame, 0, sizeof(trap_frame_t));

    // --- THE RING 3 SETUP ---
    frame->rip = code_virt;
    frame->cs  = USER_CS;     // Ring 3 Code
    frame->r_flags = 0x202;    // Interrupts Enabled
    frame->rsp = u_stack_top; // Point to USER Stack!
    frame->ss  = USER_DS;     // Ring 3 Data

    // Initialize segments to User Data (important for some CPUs)
    // If your trap_frame doesn't have these, you might need to push them in asm
    // frame->ds = USER_DS; 
    // frame->es = USER_DS;

    // 5. Context Switch Setup (The "glue" logic)
    // This matches your existing logic for kernel threads
    k_stack_top -= sizeof(uint64_t);
    *(uint64_t*)k_stack_top = (uint64_t)interrupt_return;
    
    // Reserve space for the registers popped by 'interrupt_return' (r15..r12, etc)
    // Adjust '6' to however many regs your context switch pops before iretq
    k_stack_top -= sizeof(uint64_t) * 6; 

    // 6. Save State in TCB
    new_task->rsp = k_stack_top; // Scheduler switches KERNEL stacks
    new_task->pid = next_pid++;
    new_task->task_state = TASK_READY;
    new_task->pml4 = get_kernel_page_table(); // Share VM for now
    
    // Initialize Heap for sbrk (User standard: after kernel end)
    new_task->heap_end = ((uint64_t)&_KernelEnd + 0xFFF) & ~0xFFF;
    new_task->stack_base = k_stack_bottom; // Remember for free()

    // 7. Add to List (Circular)
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