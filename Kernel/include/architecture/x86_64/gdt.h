#pragma once


typedef struct {
    unsigned short int limit_0;
    unsigned short int base_0;
    unsigned char base_1;
    unsigned char access_byte;
    unsigned char limit_1_flags;
    unsigned char base_2;
}__attribute__((packed)) gdt_entry_t;

typedef struct {
    unsigned short int limit;
    unsigned long long int offset;
} __attribute__((packed)) gdt_descriptor_t;

typedef struct {
    gdt_entry_t null;
    gdt_entry_t kernel_code;
    gdt_entry_t kernel_data;
    gdt_entry_t user_null;
    gdt_entry_t user_code;
    gdt_entry_t user_data;
    gdt_entry_t tss_1;
    gdt_entry_t tss_2;
} gdt_t;

void gdt_init();
void _LoadGDT(gdt_descriptor_t* gdt_descriptor);
