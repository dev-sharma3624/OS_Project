#include "io.h"

void io_out_b(unsigned short int port, unsigned char value){
    __asm__ volatile ("outb %0, %1" : : "a"(value), "Nd"(port));
}

unsigned char io_in_b(unsigned short int port){
    unsigned char returnVal;
    __asm__ volatile ("inb %1, %0" : "=a"(returnVal) : "Nd"(port));
    return returnVal;
}

void io_wait(){
    __asm__ volatile ("outb %%al, $0x80" : : "a"(0));
}