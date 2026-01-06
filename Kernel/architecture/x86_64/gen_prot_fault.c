#include <typedefs.h>
#include <architecture/x86_64/interrupt_handlers.h>
#include <libs/k_printf.h>
#include <architecture/x86_64/trap_frame.h>

void gpf_handler(trap_frame_t* frame) {
    // 1. Disable Interrupts (Stop the world)
    asm volatile("cli");

    k_printf("\n\n------------------------------------------------\n");
    k_printf("!!! GENERAL PROTECTION FAULT (Vector 13) !!!\n");
    k_printf("------------------------------------------------\n");
    
    k_printf("Error Code: %x\n", frame->error_code);
    k_printf("RIP: %x  (Instruction that failed)\n", frame->rip);
    k_printf("CS : %x\n", frame->cs);
    k_printf("RFLAGS: %x\n", frame->r_flags);
    k_printf("RSP: %x  (Stack Pointer at crash)\n", frame->rsp);
    k_printf("SS : %x\n", frame->ss);

    // 4. Halt the CPU forever
    k_printf("\nSystem Halted.\n");
    while(1) {
        asm volatile("hlt");
    }
}