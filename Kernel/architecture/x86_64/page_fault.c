#include <typedefs.h>
#include <architecture/x86_64/interrupt_handlers.h>
#include <architecture/x86_64/trap_frame.h>
#include <libs/k_printf.h>

void page_fault_handler(trap_frame_t* frame) {
    
    uint64_t bad_address;
    asm volatile("mov %%cr2, %0" : "=r" (bad_address));

    k_printf("--- PAGE FAULT ---\n");
    k_printf("Crash Address: %x", bad_address);
    k_printf("\nInstruction: %x", frame->rip);
    k_printf("\nError Code: %x", frame->error_code);
    
    k_printf("\nSYSTEM HALTED.");
    while(1) asm volatile("hlt");
}