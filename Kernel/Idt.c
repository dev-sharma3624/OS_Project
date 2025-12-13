#include "Idt.h"
#include "kprintf.h"

IdtDescEntry mainIdt[256];

Idtr idtr;

void SetIdtGate(unsigned char vector, void* isr, unsigned char flags){
    IdtDescEntry* entry = &mainIdt[vector];
    unsigned long long addr = (unsigned long long)isr;

    entry->offset0 = (unsigned short int) (addr & 0xFFFF);
    entry->offest1 = (unsigned short int) ((addr >> 16) & 0xFFFF);
    entry->offset2 = (unsigned int) ((addr >> 32) & 0xFFFFFFFF);

    entry->selector = 0x08;

    entry->ist = 0;

    entry->typeAttribute = flags;

    entry->ignore = 0;
}

extern void LoadIdt(Idtr* idtrPtr);


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

void InitializeIdt(){
    idtr.limit = (sizeof(IdtDescEntry) * 256) - 1;
    idtr.offset = (unsigned long long)&mainIdt;

    SetIdtGate(0, DummyHandler, InterruptGate);

    LoadIdt(&idtr);
}