#include <typedefs.h>
#include <architecture/x86_64/interrupt_handlers.h>
#include <architecture/x86_64/spinlock_atomic_instr.h>
#include <libs/k_printf.h>

#define SYS_PRINT 0

void syscall_dispatcher(trap_frame_t* frame){
    uint64_t syscall_no = frame->rax;

    switch (syscall_no) {
        case SYS_PRINT:
            char* user_string = (char*)frame->rbx;
            k_printf(user_string);
            
            frame->rax = 0; 
            break;

        default:
            k_printf("Unknown Syscall: %d\n", syscall_no);
            frame->rax = -1;
            break;
    }


}