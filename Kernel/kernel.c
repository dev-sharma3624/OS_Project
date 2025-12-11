#include "../boot_info.h"
#include "BasicRenderer.h"

void __attribute__((ms_abi)) kernelStart(BOOT_INFO* bootInfo){

    if(!bootInfo) return;

    FrameBuffer frameBuffer = bootInfo->frameBuffer;
    PSF1_FONT* font = bootInfo->font;


    BasicRenderer basicRenderer;
    BasicRenderer_Init(&basicRenderer, &frameBuffer, font);
    basicRenderer.clearColor = 0xFFFF8000;
    basicRenderer.color = 0xff000000;
    
    BasicRenderer_ClearScreen(&basicRenderer);
    BasicRenderer_Print(&basicRenderer, "Print is working\n");
    BasicRenderer_Print(&basicRenderer, "Font renderer is working!");

    /*

    ................................................................................................
    ................................................................................................

    This version of kernel was meant to change the color of entire screen by directly manipulating
    the frameBuffer data by iterating from frameBufferBase to framebuffer size.

    ................................................................................................
    ................................................................................................
    
     unsigned int* frameBufferBase = (unsigned int*)bootInfo->frameBuffer.frameBufferBase;
    unsigned int frameBufferSize = bootInfo->frameBuffer.frameBufferSize;

    unsigned int color = 0xFF0000FF;

    for(unsigned int i = 0; i < frameBufferSize / 4; i++){
        frameBufferBase[i] = color;
    }
 */
    while (1)
    {
        __asm__ ("hlt");
    }
    

}


/* 

gcc -ffreestanding -mno-red-zone -mgeneral-regs-only -I. -c Kernel/kernel.c -o Kernel/kernel.o
gcc -ffreestanding -mno-red-zone -mgeneral-regs-only -I. -c Kernel/BasicRenderer.c -o Kernel/BasicRenderer.o

ld -o kernel.elf -Ttext 0x8000000 -e kernelStart Kernel/kernel.o
ld -Ttext 0x8000000 --entry kernelStart -o kernel.elf Kernel/kernel.o Kernel/BasicRenderer.o

 */