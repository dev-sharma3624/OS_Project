#pragma once
#include <typedefs.h>

typedef struct {
    unsigned long int ip;
    unsigned long int cs;
    unsigned long int flags;
    unsigned long int sp;
    unsigned long int ss;
} interrupt_frame_t;

__attribute__((interrupt)) void keyboard_driver_handler(interrupt_frame_t* frame);
__attribute__((interrupt)) void page_fault_handler(interrupt_frame_t* frame, uint64_t error_code);