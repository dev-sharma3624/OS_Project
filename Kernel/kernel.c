#include "../boot_info.h"
#include "BasicRenderer.h"
#include "kprintf.h"
BOOT_INFO bootInfo;

void __attribute__((ms_abi)) kernelStart(BOOT_INFO* bootInfo_recieved){

    if(!bootInfo_recieved) return;

    bootInfo = *bootInfo_recieved;

    // 1. Initialize the Global Renderer
    BasicRenderer_Init(&frameBuffer, font, 0xff000000, 0xFFFF8000);

    // 2. Clear the screen (Blue background usually, or whatever your default is)
    BasicRenderer_ClearScreen();

    // 3. Test 1: Basic String & Char
    kPrintf("Test 1: Hello World! %c\n", 'A');

    // 4. Test 2: Decimal Integers (Positive & Negative)
    kPrintf("Test 2: Integer: %d, Negative: %d\n", 1234, -5678);

    // 5. Test 3: Hexadecimal
    // 255 should print as 0xFF (or 0xff depending on your implementation)
    kPrintf("Test 3: Hex: %x\n", 255);

    // 6. Test 4: Pointers
    // This prints the memory address of the bootInfo struct
    kPrintf("Test 4: Pointer Address: %p\n", bootInfo);

    // 7. Test 5: Mixed Formatting
    kPrintf("Test 5: Mixed: %s is %d years old.\n", "Dev", 21);


    /* 

    ................................................................................................
    ................................................................................................

    This version of kernel was meant to test the BasicRenderer methods that would facilitate printing
    of strings on the screen using the psf1 font.

    ................................................................................................
    ................................................................................................
    
    BasicRenderer_Init(&frameBuffer, font, 0xff000000, 0xFFFF8000);
    
    BasicRenderer_ClearScreen();
    BasicRenderer_Print("Print is working\n");
    BasicRenderer_Print("Font renderer is working!"); */

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