#include <architecture/x86_64/idt.h>
#include <libs/k_printf.h>
#include <drivers/keyboard_driver.h>

idt_desc_entry_t main_idt[256];

idtr_t idtr;

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

    idt_set_gate(33, keyboard_driver_handler, INTERRUPT_GATE);

    _LoadIdt(&idtr);
}