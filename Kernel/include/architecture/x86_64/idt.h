#pragma once

enum IDT_ATTRIBUTES{
    INTERRUPT_GATE = 0b10001110,
    CALL_GATE = 0b10001100,
    TRAP_GATE = 0b10001111
};

typedef struct{
    unsigned short int offset_0;
    unsigned short int selector;
    unsigned char ist;
    unsigned char type_attribute;
    unsigned short int offest_1;
    unsigned int offset_2;
    unsigned int ignore;
} __attribute__ ((packed)) idt_desc_entry_t;

typedef struct {
    unsigned short int limit;
    unsigned long long offset;
} __attribute__ ((packed)) idtr_t;

void idt_init();