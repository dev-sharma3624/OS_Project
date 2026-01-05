#pragma once
#include <typedefs.h>
#include <architecture/x86_64/trap_frame.h>

/* typedef struct {
    unsigned long int ip;
    unsigned long int cs;
    unsigned long int flags;
    unsigned long int sp;
    unsigned long int ss;
} interrupt_frame_t; */

void keyboard_driver_handler();
void page_fault_handler(trap_frame_t* frame);
void timer_handler();