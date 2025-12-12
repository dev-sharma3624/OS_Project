#include "Gdt.h"

Gdt defaultGdt;

GdtDescriptor gdtDescriptor;

void InitializeGDT(){

    gdtDescriptor.limit = sizeof(Gdt) - 1;

    gdtDescriptor.offset = (unsigned long long int)&defaultGdt;

    defaultGdt.null.limit0 = 0;
    defaultGdt.null.base0 = 0;
    defaultGdt.null.base1 = 0;
    defaultGdt.null.accessByte = 0;
    defaultGdt.null.limit1Flags = 0;
    defaultGdt.null.base2 = 0;

    defaultGdt.kernelCode.limit0 = 0xFFFF;
    defaultGdt.kernelCode.base0 = 0;
    defaultGdt.kernelCode.base1 = 0;
    defaultGdt.kernelCode.accessByte = 0x9A;
    defaultGdt.kernelCode.limit1Flags = 0xAF;
    defaultGdt.kernelCode.base2 = 0;

    defaultGdt.kernelData.limit0 = 0xFFFF;
    defaultGdt.kernelData.base0 = 0;
    defaultGdt.kernelData.base1 = 0;
    defaultGdt.kernelData.accessByte = 0X92;
    defaultGdt.kernelData.limit1Flags = 0xCF;
    defaultGdt.kernelData.base2 = 0;

    defaultGdt.userNull.limit0 = 0;
    defaultGdt.userNull.base0 = 0;
    defaultGdt.userNull.base1 = 0;
    defaultGdt.userNull.accessByte = 0;
    defaultGdt.userNull.limit1Flags = 0;
    defaultGdt.userNull.base2 = 0;

    defaultGdt.userCode.limit0 = 0xFFFF;
    defaultGdt.userCode.base0 = 0;
    defaultGdt.userCode.base1 = 0;
    defaultGdt.userCode.accessByte = 0xFA;
    defaultGdt.userCode.limit1Flags = 0xAF;
    defaultGdt.userCode.base2 = 0;

    defaultGdt.userData.limit0 = 0xFFFF;
    defaultGdt.userData.base0 = 0;
    defaultGdt.userData.base1 = 0;
    defaultGdt.userData.accessByte = 0xF2;
    defaultGdt.userData.limit1Flags = 0xCF;
    defaultGdt.userData.base2 = 0;

    LoadGDT(&gdtDescriptor);

}