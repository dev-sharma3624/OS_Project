#include "../boot_info.h"
#include "BasicRenderer.h"
#include "kprintf.h"
#include "Gdt.h"
#include "Idt.h"
#include "PIC.h"
#include "Keyboard.h"
#include "KeyboardMap.h"
#include "Memory.h"
#include "Typedefs.h"
#include "PageFrameAllocator.h"

#define MAX_COMMAND_BUFFER 256
char commandBuffer[MAX_COMMAND_BUFFER];
int bufferPosition = 0;

BOOT_INFO bootInfo;

int strcmp(const char* str1, const char* str2){

    while(*str1 && (*str1 == *str2)){
        str1++;
        str2++;
    }

    return *(const unsigned char*) str1 - *(const unsigned char*)str2;

}

void clearBuffer(){
    for(int i = 0; i < MAX_COMMAND_BUFFER; i++){
        commandBuffer[i] = 0;
    }
    bufferPosition = 0;
}


void PrintMemoryInfo() {
    uint64_t totalMemory = freeMemory + reservedMemory + usedMemory;

    kPrintf("\n--- Memory Statistics ---\n");
    kPrintf("Total RAM:    %d MB\n", totalMemory / 1024 / 1024);
    kPrintf("Free RAM:     %d MB\n", freeMemory / 1024 / 1024);
    kPrintf("Used RAM:     %d KB\n", usedMemory / 1024);
    kPrintf("Reserved RAM: %d MB\n", reservedMemory / 1024 / 1024);

    kPrintf("-------------------------\n");
}


void executeCommand(){
    kPrintf("\n");

    if(strcmp(commandBuffer, "help") == 0){
        kPrintf("Available commands:\n");
        kPrintf(" - help: Show this menu\n");
        kPrintf(" - clear: Clean the screen\n");
        kPrintf(" - reboot: Reboot the CPU\n");
        kPrintf(" - meminfo: See how much RAM is being used\n");
        kPrintf(" - drums of liberation: Awaken the Sun God!\n");
    }

    else if(strcmp(commandBuffer, "clear") == 0){
        BasicRenderer_ClearScreen();
    }

    else if(strcmp(commandBuffer, "meminfo") == 0){
        PrintMemoryInfo();
    }

    else if(strcmp(commandBuffer, "drums of liberation") == 0){
        kPrintf("THE ONE PIECE IS REAL!\n");
    }

    else if (bufferPosition > 0){
        kPrintf("Unknown command: ");
        kPrintf("%s\n", commandBuffer);
    };

    clearBuffer();
    kPrintf("Project D> ");
    
}

void kernelStart(BOOT_INFO* bootInfo_recieved){

    if(!bootInfo_recieved) return;

    bootInfo = *bootInfo_recieved;

    BasicRenderer_Init(&bootInfo.frameBuffer, bootInfo.font, 0xff000000, 0xFFFF8000);

    BasicRenderer_ClearScreen();

    kPrintf("Kernel initialized.\n");
    kPrintf("Loading GDT...\n");

    InitializeGDT();

    kPrintf("GDT loaded successfully\n");

    kPrintf("Loading IDT...\n");

    InitializeIdt();
    kPrintf("IDT loaded successfully\n");

    kPrintf("Remapping PIC...\n");
    remapPIC();
    kPrintf("PIC remapping successfull\n");

    uint64_t mMapSize = GetMemorySize(&bootInfo);

    kPrintf("Total memory: %p bytes\n", mMapSize);
    kPrintf("Total memory: %d MB\n", (int) mMapSize / 1024 / 1024);

    InitPageFrameAllocator(&bootInfo);
    kPrintf("Page Frame Allocator Initialized!\n");

    void* page1 = RequestPage();
    kPrintf("Page 1 request: %p\n", (uint64_t)page1);

    void* page2 = RequestPage();
    kPrintf("Page 2 request: %p\n", (uint64_t)page2);

    void* page3 = RequestPage();
    kPrintf("Page 3 Request: %p\n", (uint64_t)page3);

    void* page4= RequestPage();
    kPrintf("Page 4 Request: %p\n", (uint64_t)page4);
    
    kPrintf("Freeing Page 1...\n");
    FreePage(page1);

    void* page5 = RequestPage();
    kPrintf("Page 5 Request: %p\n", (uint64_t)page5);


    if (page5 == page1) {
        kPrintf("TEST PASSED: Memory Recycled!\n");
    } else {
        kPrintf("TEST FAILED: Allocator ignored the free page.\n");
    }

    __asm__ volatile ("sti"); 

    kPrintf("Input enabled.\n\n");

    kPrintf("Project D v0.1. Type 'help'.\nProject D> ");
    
    while (1)
    {
        unsigned char scanCode = ReadKey();
        if(scanCode != 0){

            if(scanCode & 0x80){
                continue;
            }
            
            if(scanCode == 0x0E){

                if(bufferPosition > 0){

                    bufferPosition--;
                    commandBuffer[bufferPosition] = 0;


                    kPrintf("\b");
                }
            }

            else if (scanCode == 0x1C){
                executeCommand();
            }

            if(scanCode < 0x3A){
                char ascii = scanCodeLookupTable[scanCode];

                if(ascii != 0){

                    if(bufferPosition < MAX_COMMAND_BUFFER - 1){

                        kPrintf("%c", ascii);
                        commandBuffer[bufferPosition] = ascii;
                        bufferPosition++;

                    }
                }
            }
        }

        __asm__ volatile ("hlt");

    }
}

    

    /*

    ................................................................................................
    ................................................................................................

    Here, we successfully used the memory data passed from the bootloader to calculate the total
    available memory(RAM).

    ................................................................................................
    ................................................................................................ 

    BasicRenderer_Init(&bootInfo.frameBuffer, bootInfo.font, 0xff000000, 0xFFFF8000);

    BasicRenderer_ClearScreen();

    kPrintf("Kernel initialized.\n");
    kPrintf("Loading GDT...\n");

    InitializeGDT();

    kPrintf("GDT loaded successfully\n");

    kPrintf("Loading IDT...\n");

    InitializeIdt();
    kPrintf("IDT loaded successfully\n");

    kPrintf("Remapping PIC...\n");
    remapPIC();
    kPrintf("PIC remapping successfull\n");

    uint64_t mMapSize = GetMemorySize(&bootInfo);

    kPrintf("Total memory: %p bytes\n", mMapSize);
    kPrintf("Total memory: %d MB\n", (int) mMapSize / 1024 / 1024);

    __asm__ volatile ("sti"); 

    kPrintf("Input enabled.\n\n");

    kPrintf("Project D v0.1. Type 'help'.\nProject D> ");
    
    while (1)
    {
        unsigned char scanCode = ReadKey();
        if(scanCode != 0){

            if(scanCode & 0x80){
                continue;
            }
            
            if(scanCode == 0x0E){

                if(bufferPosition > 0){

                    bufferPosition--;
                    commandBuffer[bufferPosition] = 0;


                    kPrintf("\b");
                }
            }

            else if (scanCode == 0x1C){
                executeCommand();
            }

            if(scanCode < 0x3A){
                char ascii = scanCodeLookupTable[scanCode];

                if(ascii != 0){

                    if(bufferPosition < MAX_COMMAND_BUFFER - 1){

                        kPrintf("%c", ascii);
                        commandBuffer[bufferPosition] = ascii;
                        bufferPosition++;

                    }
                }
            }
        }

        __asm__ volatile ("hlt");

    } */

    /* 

    ................................................................................................
    ................................................................................................

    This version of kernel had a basic working shell with few commands working!

    ................................................................................................
    ................................................................................................
    
    BasicRenderer_Init(&bootInfo.frameBuffer, bootInfo.font, 0xff000000, 0xFFFF8000);

    BasicRenderer_ClearScreen();

    kPrintf("Kernel initialized.\n");
    kPrintf("Loading GDT...\n");

    InitializeGDT();

    kPrintf("GDT loaded successfully\n");

    kPrintf("Loading IDT...\n");

    InitializeIdt();
    kPrintf("IDT loaded successfully\n");

    kPrintf("Remapping PIC...\n");
    remapPIC();
    kPrintf("PIC remapping successfull\n");

    __asm__ volatile ("sti"); 

    kPrintf("Input enabled.\n\n\n\n");

    kPrintf("DevOS v0.1. Type 'help'.\nDevOS> ");
    
    while (1)
    {
        unsigned char scanCode = ReadKey();
        if(scanCode != 0){

            if(scanCode & 0x80){
                continue;
            }
            
            if(scanCode == 0x0E){

                if(bufferPosition > 0){

                    bufferPosition--;
                    commandBuffer[bufferPosition] = 0;


                    kPrintf("\b");
                }
            }

            else if (scanCode == 0x1C){
                executeCommand();
            }

            if(scanCode < 0x3A){
                char ascii = scanCodeLookupTable[scanCode];

                if(ascii != 0){

                    if(bufferPosition < MAX_COMMAND_BUFFER - 1){

                        kPrintf("%c", ascii);
                        commandBuffer[bufferPosition] = ascii;
                        bufferPosition++;

                    }
                }
            }
        }

        __asm__ volatile ("hlt");

    } */

    /* 

    ................................................................................................
    ................................................................................................

    This version of kernel was successfully able to print text on screen from keyboard interrupts.
    Keyboard became alive here! 

    ................................................................................................
    ................................................................................................
    
    BasicRenderer_Init(&bootInfo.frameBuffer, bootInfo.font, 0xff000000, 0xFFFF8000);

    BasicRenderer_ClearScreen();

    kPrintf("Kernel initialized.\n");
    kPrintf("Loading GDT...\n");

    InitializeGDT();

    kPrintf("GDT loaded successfully\n");

    kPrintf("Loading IDT...\n");

    InitializeIdt();
    kPrintf("IDT loaded successfully\n");

    kPrintf("Remapping PIC...\n");
    remapPIC();
    kPrintf("PIC remapping successfull\n");

    __asm__ volatile ("sti"); 

    kPrintf("Input enabled.\n\n");
    
    while (1)
    {
        unsigned char scanCode = ReadKey();
        if(scanCode != 0){
            if(scanCode < 0x3A){
                char ascii = scanCodeLookupTable[scanCode];

                if(ascii != 0){
                    kPrintf("%c", ascii);
                }
            }
        }

    } */

    /* 

    ................................................................................................
    ................................................................................................

    This version of kernel successfully remapped PIC to move hardware interrupts from 0-15 to 32-47

    ................................................................................................
    ................................................................................................
    
    BasicRenderer_Init(&bootInfo.frameBuffer, bootInfo.font, 0xff000000, 0xFFFF8000);

    BasicRenderer_ClearScreen();

    kPrintf("Kernel initialized.\n");
    kPrintf("Loading GDT...\n");

    InitializeGDT();

    kPrintf("GDT loaded successfully\n");

    kPrintf("Loading IDT...\n");

    InitializeIdt();
    kPrintf("IDT loaded successfully\n");

    kPrintf("Remapping PIC...\n");
    remapPIC();
    kPrintf("PIC remapping successfull\n");
    while (1)
    {
        __asm__ ("hlt");
    } */


    /* 

    ................................................................................................
    ................................................................................................

    This version of kernel was loading the GDT and IDT and testing a "Divide by zero" error that
    triggers vector 0 of the interrupt table.

    ................................................................................................
    ................................................................................................

    
    BasicRenderer_Init(&bootInfo.frameBuffer, bootInfo.font, 0xff000000, 0xFFFF8000);

    BasicRenderer_ClearScreen();

    kPrintf("Kernel initialized.\n");
    kPrintf("Loading GDT...\n");

    InitializeGDT();

    kPrintf("GDT loaded successfully\n");

    kPrintf("Loading IDT...\n");

    InitializeIdt();
    kPrintf("IDT loaded successfully\n");

    kPrintf("Preparing to crash...\n");

    volatile int a = 10;
    volatile int b = 0;
    int c = a / b;

    kPrintf("If you see this, the OS survived (which is bad!)\n"); 


    ***********************************************************************************************
    ***********************************************************************************************

    This is the dummy handler function that was getting triggered on divide by zero exception in this
    version of kernel. Keeping it here in case it gets deleted from Idt.c file in later versions.

    void DummyHandler() {
        kPrintf("\n");
        kPrintf("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
        kPrintf("!!!      INTERRUPT RECEIVED          !!!\n");
        kPrintf("!!!   Vector 0: Divide By Zero       !!!\n");
        kPrintf("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
        
        // Halt the CPU forever so we can read the message
        while(1) {
            __asm__ volatile ("hlt");
        }
    }


    ***********************************************************************************************
    ***********************************************************************************************
    
    */

    /* 

    ................................................................................................
    ................................................................................................

    This version of kernel was loading the Global Descriptor Table to the CPU registers and verifying
    the code segment register value to verfiy whether our GDT was loaded successfully or not.

    ................................................................................................
    ................................................................................................

    BasicRenderer_Init(&bootInfo.frameBuffer, bootInfo.font, 0xff000000, 0xFFFF8000);
    
    BasicRenderer_ClearScreen();

    kPrintf("Kernel initialized.\n");
    kPrintf("Loading GDT...\n");

    InitializeGDT();

    unsigned short int currentCS;

    __asm__ volatile ("mov %%cs, %0" : "=r"(currentCS));

    kPrintf("Current CS Register: %x\n", currentCS);

    kPrintf("GDT loaded successfully"); */
    

    /*

    ................................................................................................
    ................................................................................................

    This version of kernel was meant to test the kPrintf() method that will help us print variables
    like decimals, hexadecimals etc to the screen thus helping us in printing debugging messages.

    ................................................................................................
    ................................................................................................

    FrameBuffer frameBuffer = bootInfo.frameBuffer;
    PSF1_FONT* font = bootInfo.font;

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
    kPrintf("Test 5: Mixed: %s is %d years old.\n", "Dev", 21); */


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