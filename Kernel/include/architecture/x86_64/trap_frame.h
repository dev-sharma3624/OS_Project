#pragma once
#include <typedefs.h>


/* 
this saves all the registers that are needed to save the current state of process onto its stack

the order in which registers are mentioned should match exactly with the assembly code that is pushing
the registers on the stack

first r15 will be pushed, then r14,.., lastly ss will be pushed

stack grows downwards so r15 will be at the bottom and ss will be at the top of stack
*/
typedef struct trap_frame_t {

    // general purpose registers used by any running process
    uint64_t r15;
    uint64_t r14;
    uint64_t r13;
    uint64_t r12;
    uint64_t r11;
    uint64_t r10;
    uint64_t r9;
    uint64_t r8;
    uint64_t rbp;
    uint64_t rdi;
    uint64_t rsi;
    uint64_t rdx;
    uint64_t rcx;
    uint64_t rbx;
    uint64_t rax;

    uint64_t interrupt_no; // which interrupt triggerd switch
    uint64_t error_code;

    // these are pushed automatically by cpu
    uint64_t rip; //instruction pointer
    uint64_t cs; // code segment
    uint64_t r_flags; // cpu flags
    uint64_t rsp; // stack pointer
    uint64_t ss; // stack segment
} __attribute__((packed)) trap_frame_t;