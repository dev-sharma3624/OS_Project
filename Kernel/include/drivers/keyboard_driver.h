#pragma once

#include <architecture/x86_64/interrupt_frame.h>

__attribute__((interrupt)) void keyboard_driver_handler(interrupt_frame_t* frame);
unsigned char read_key();




