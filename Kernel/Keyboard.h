#pragma once

typedef struct {
    unsigned long int ip;
    unsigned long int cs;
    unsigned long int flags;
    unsigned long int sp;
    unsigned long int ss;
} InterruptFrame;

__attribute__((interrupt)) void KeyboardHandler(InterruptFrame* frame);
unsigned char ReadKey();




