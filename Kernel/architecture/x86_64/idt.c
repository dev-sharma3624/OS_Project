#include <architecture/x86_64/idt.h>
#include <libs/k_printf.h>
#include <architecture/x86_64/interrupt_handlers.h>
#include <architecture/x86_64/trap_frame.h>
#include <architecture/x86_64/pic.h>
#include <architecture/x86_64/io.h>

idt_desc_entry_t main_idt[256];

idtr_t idtr;

extern void isr6(void);
extern void isr13(void);
extern void isr14(void);
extern void isr32(void);
extern void isr33(void);

void idt_set_gate(unsigned char vector, void* isr, unsigned char flags){
    idt_desc_entry_t* entry = &main_idt[vector];
    unsigned long long addr = (unsigned long long)isr;

    entry->offset_0 = (unsigned short int) (addr & 0xFFFF);
    entry->offest_1 = (unsigned short int) ((addr >> 16) & 0xFFFF);
    entry->offset_2 = (unsigned int) ((addr >> 32) & 0xFFFFFFFF);

    entry->selector = 0x08;

    entry->ist = 0;

    entry->type_attribute = flags;

    entry->ignore = 0;
}

extern void _LoadIdt(idtr_t* idt_ptr);


void idt_dummy_handler() {
    k_printf("\n");
    k_printf("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
    k_printf("!!!      INTERRUPT RECEIVED          !!!\n");
    k_printf("!!!   Vector 0: Divide By Zero       !!!\n");
    k_printf("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
    
    // Halt the CPU forever so we can read the message
    while(1) {
        __asm__ volatile ("hlt");
    }
}

void idt_init(){
    idtr.limit = (sizeof(idt_desc_entry_t) * 256) - 1;
    idtr.offset = (unsigned long long)&main_idt;

    idt_set_gate(6, isr6, INTERRUPT_GATE);
    idt_set_gate(13, isr13, INTERRUPT_GATE);
    idt_set_gate(14, isr14, INTERRUPT_GATE);
    idt_set_gate(32, isr32, INTERRUPT_GATE);
    idt_set_gate(33, isr33, INTERRUPT_GATE);

    _LoadIdt(&idtr);
}

void isr_handler(trap_frame_t* trap_frame){

    // hardware interrupts
    if(trap_frame->interrupt_no >= 32 && trap_frame->interrupt_no <= 47){

        pic_send_eoi(trap_frame->interrupt_no - 32);

        switch (trap_frame->interrupt_no) {

            case 32:
                timer_handler();
                break;

            case 33:
                keyboard_driver_handler();
                break;
            
            default:
                break;
        }

    }

    // cpu exceptions
    else if(trap_frame->interrupt_no < 32){

        switch (trap_frame->interrupt_no) {

            case 6:
                io_print("higer jump address: ");
                print_address_hex(trap_frame->rip);
                // k_printf("PANIC: Invalid Opcode at RIP: %x\n", trap_frame->rip);
                while(1);
                break;

            case 13:
                gpf_handler(trap_frame);
                break;

            case 14:
                page_fault_handler(trap_frame);
                break;
            
            default:
                break;
        }

    }

}