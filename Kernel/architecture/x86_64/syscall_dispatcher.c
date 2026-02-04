#include <typedefs.h>
#include <architecture/x86_64/interrupt_handlers.h>
#include <architecture/x86_64/spinlock_atomic_instr.h>
#include <libs/k_printf.h>
#include <drivers/keyboard_driver.h>
#include <cpu_scheduling/process.h>
#include <cpu_scheduling/scheduler.h>
#include <memory_management/paging.h>
#include <memory_management/pmm.h>
#include <file_system/fs_interface.h>

extern tcb_t* current_task;

enum SYSTEM_CALL_NOS{
    SYS_PRINT,
    SYS_READ,
    SYS_EXIT,
    SYS_SBRK,
    SYS_LS,
    SYS_CREATE_FILE
};

void dispatch_print(trap_frame_t* frame){

    char* user_string = (char*)(frame->rbx);
    k_printf(user_string);
    
    frame->rax = 0;
}

void dispatch_read(trap_frame_t* frame){

    char* user_buffer = (char*)frame->rdi; 
    
    char key = read_key(); 
    
    *user_buffer = key;
    
    frame->rax = 1;
}

void dispatch_exit(trap_frame_t* frame){
    int exit_code = (int)frame->rdi;
    
    k_printf("\n[Kernel] Process %d exiting with code %d.\n", current_task->pid, exit_code);

    current_task->task_state = TASK_ZOMBIE;

    schedule(); 

    k_printf("[Kernel] Panic: Dead task was scheduled again!\n");
    while(1);
}

void dispatch_brk(trap_frame_t* frame){
    int64_t increment = (int64_t)frame->rdi;
            
    uint64_t old_break = current_task->heap_end;
    
    uint64_t new_break = old_break + increment;
    
    uint64_t start_page = (old_break + 0xFFF) & ~0xFFF;
    uint64_t end_page   = (new_break + 0xFFF) & ~0xFFF;
    
    for (uint64_t page = start_page; page < end_page; page += 4096) {

        void* phys_frame = pmm_request_page();

        if (!phys_frame) {
            k_printf("OOM during sbrk!\n");
            frame->rax = -1;
            return; 
        }

        paging_map_page(current_task->pml4, page, (uint64_t)phys_frame, PT_FLAG_PRESENT | PT_FLAG_READ_WRITE | PT_FLAG_USER_SUPER);
    }

    current_task->heap_end = new_break;
    frame->rax = old_break;
}

void dispatch_ls(trap_frame_t* frame){

    k_printf("\nfat32 call comes here\n");
    
    frame->rax = 0;
}

void dispatch_create_file(trap_frame_t* frame){
    char* file_name = (char*)frame->rdi;
    char* content = (char*) frame->rsi;
    int result = fs_create_file(file_name, content);
}

void syscall_dispatcher(trap_frame_t* frame){
    uint64_t syscall_no = frame->rax;

    switch (syscall_no) {
        case SYS_PRINT:
            dispatch_print(frame);
            break;

        case SYS_READ:
            dispatch_read(frame);
            break;

        case SYS_EXIT:
            dispatch_exit(frame);
            break;

        case SYS_SBRK:
            dispatch_brk(frame);
            break;

        case SYS_LS:
            dispatch_ls(frame);
            break;

        case SYS_CREATE_FILE:
            dispatch_create_file(frame);
            break;

        default:
            k_printf("Unknown Syscall: %d\n", syscall_no);
            frame->rax = -1;
            break;
    }


}