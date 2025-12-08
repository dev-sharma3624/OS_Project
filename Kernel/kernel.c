#include "boot_info.h"

void __attribute__((ms_abi)) kernelStart(BOOT_INFO* bootInfo){

    if(!bootInfo) return;

    unsigned int* frameBufferBase = (unsigned int*)bootInfo->frameBufferBase;
    unsigned int frameBufferSize = bootInfo->frameBufferSize;

    unsigned int color = 0xFF0000FF;

    for(unsigned int i = 0; i < frameBufferSize / 4; i++){
        frameBufferBase[i] = color;
    }

    while (1)
    {
        __asm__ ("hlt");
    }
    

}


/* 

gcc -ffreestanding -mno-red-zone -I. -c Kernel/kernel.c -o Kernel/kernel.o


ld -o kernel.elf -Ttext 0x8000000 -e kernelStart Kernel/kernel.o

 */