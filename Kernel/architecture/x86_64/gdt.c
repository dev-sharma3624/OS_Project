#include <architecture/x86_64/gdt.h>

gdt_t default_gdt;

gdt_descriptor_t gdt_descriptor;

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

    _LoadGDT(&gdt_descriptor);

}