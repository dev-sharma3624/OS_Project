#include <typedefs.h>
#include <architecture/x86_64/interrupt_handlers.h>
#include <architecture/x86_64/spinlock_atomic_instr.h>
#include <libs/k_printf.h>
#include <drivers/keyboard_driver.h>
#include <drivers/keyboard_map.h>

enum SYSTEM_CALL_NOS{
    SYS_PRINT,
    SYS_READ
};

void syscall_dispatcher(trap_frame_t* frame){
    uint64_t syscall_no = frame->rax;

    switch (syscall_no) {
        case SYS_PRINT:
            char* user_string = (char*)frame->rbx;
            k_printf(user_string);
            
            frame->rax = 0; 
            break;

        case SYS_READ:
            char* user_buffer = (char*)frame->rdi; 
            
            unsigned char key = read_key(); 
            
            *user_buffer = scan_code_for_lookup_table[key];
            
            frame->rax = 1;
            break;

        default:
            k_printf("Unknown Syscall: %d\n", syscall_no);
            frame->rax = -1;
            break;
    }


}