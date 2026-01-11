#include <typedefs.h>
#include <architecture/x86_64/interrupt_handlers.h>
#include <architecture/x86_64/trap_frame.h>
#include <libs/k_printf.h>
#include <architecture/x86_64/io.h>

void page_fault_handler(trap_frame_t* frame) {
    
    uint64_t bad_address;
    asm volatile("mov %%cr2, %0" : "=r" (bad_address));

    io_print("Page fault\n");
    print_address_hex((void*)bad_address);

    io_print("Instruction: ");
    print_address_hex((void*)frame->rip);
    io_print("Error code: ");
    print_address_hex((void*)frame->error_code);

    /* k_printf("--- PAGE FAULT ---\n");
    k_printf("Crash Address: %x", bad_address);
    k_printf("\nInstruction: %x", frame->rip);
    k_printf("\nError Code: %x", frame->error_code);
    
    k_printf("\nSYSTEM HALTED."); */
    while(1) asm volatile("hlt");
}