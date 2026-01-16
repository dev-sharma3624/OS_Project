#include <architecture/x86_64/gdt.h>
#include <architecture/x86_64/tss.h>
#include <architecture/x86_64/io.h>

tss_t tss = (tss_t){0};

gdt_t default_gdt;

gdt_descriptor_t gdt_descriptor;

extern uint64_t stack_top;

void gdt_init(){

    gdt_descriptor.limit = sizeof(gdt_t) - 1;

    gdt_descriptor.offset = (unsigned long long int)&default_gdt;

    default_gdt.null.limit_0 = 0;
    default_gdt.null.base_0 = 0;
    default_gdt.null.base_1 = 0;
    default_gdt.null.access_byte = 0;
    default_gdt.null.limit_1_flags = 0;
    default_gdt.null.base_2 = 0;

    default_gdt.kernel_code.limit_0 = 0xFFFF;
    default_gdt.kernel_code.base_0 = 0;
    default_gdt.kernel_code.base_1 = 0;
    default_gdt.kernel_code.access_byte = 0x9A;
    default_gdt.kernel_code.limit_1_flags = 0xAF;
    default_gdt.kernel_code.base_2 = 0;

    default_gdt.kernel_data.limit_0 = 0xFFFF;
    default_gdt.kernel_data.base_0 = 0;
    default_gdt.kernel_data.base_1 = 0;
    default_gdt.kernel_data.access_byte = 0X92;
    default_gdt.kernel_data.limit_1_flags = 0xCF;
    default_gdt.kernel_data.base_2 = 0;

    default_gdt.user_null.limit_0 = 0;
    default_gdt.user_null.base_0 = 0;
    default_gdt.user_null.base_1 = 0;
    default_gdt.user_null.access_byte = 0;
    default_gdt.user_null.limit_1_flags = 0;
    default_gdt.user_null.base_2 = 0;

    default_gdt.user_code.limit_0 = 0xFFFF;
    default_gdt.user_code.base_0 = 0;
    default_gdt.user_code.base_1 = 0;
    default_gdt.user_code.access_byte = 0xFA;
    default_gdt.user_code.limit_1_flags = 0xAF;
    default_gdt.user_code.base_2 = 0;

    default_gdt.user_data.limit_0 = 0xFFFF;
    default_gdt.user_data.base_0 = 0;
    default_gdt.user_data.base_1 = 0;
    default_gdt.user_data.access_byte = 0xF2;
    default_gdt.user_data.limit_1_flags = 0xCF;
    default_gdt.user_data.base_2 = 0;

    tss.rsp0 = (uint64_t) &stack_top;
    tss.iomap_base = sizeof(tss_t);

    uint64_t tss_base = (uint64_t) &tss;
    uint64_t tss_limit = sizeof(tss_t) - 1;

    default_gdt.tss_1.base_0    = tss_base & 0xFFFF;
    default_gdt.tss_1.base_1 = (tss_base >> 16) & 0xFF;
    default_gdt.tss_1.base_2   = (tss_base >> 24) & 0xFF;
    default_gdt.tss_1.limit_0   = tss_limit & 0xFFFF;
    default_gdt.tss_1.access_byte      = 0x89;
    default_gdt.tss_1.limit_1_flags = 0x00;

    default_gdt.tss_2.limit_0 = (tss_base >> 32) & 0xFFFF;
    default_gdt.tss_2.base_0 = (tss_base >> 48) & 0xFFFF;
    default_gdt.tss_2.base_1 = 0;
    default_gdt.tss_2.access_byte = 0;
    default_gdt.tss_2.limit_1_flags = 0;
    default_gdt.tss_2.base_2 = 0;

    _LoadGDT(&gdt_descriptor);

    asm volatile("ltr %0" : : "r" ((uint16_t)0x30));

    uint16_t tr;
    asm volatile("str %0" : "=r" (tr));

    if (tr == 0x30) {
    // Print "TSS Loaded Successfully"
        io_print("tss loaded successfully\n");
    } else {
        io_print("tss failed\n");
        // Print "TSS Failed"
    }

}