#include <typedefs.h>
#include <architecture/x86_64/interrupt_frame.h>
#include <architecture/x86_64/page_fault.h>
#include <libs/k_printf.h>

__attribute__((interrupt)) void page_fault_handler(interrupt_frame_t* frame, uint64_t error_code) {
    
    uint64_t bad_address;
    asm volatile("mov %%cr2, %0" : "=r" (bad_address));

    k_printf("--- PAGE FAULT ---\n");
    k_printf("Crash Address: %x", bad_address);
    k_printf("\nInstruction: %x", frame->ip);
    k_printf("\nError Code: %x", error_code);
    
    k_printf("\nSYSTEM HALTED.");
    while(1) asm volatile("hlt");
}