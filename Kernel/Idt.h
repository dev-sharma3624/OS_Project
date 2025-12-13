#pragma once

enum IdtAttributes{
    InterruptGate = 0b10001110,
    CallGate = 0b10001100,
    TrapGate = 0b10001111
};

typedef struct{
    unsigned short int offset0;
    unsigned short int selector;
    unsigned char ist;
    unsigned char typeAttribute;
    unsigned short int offest1;
    unsigned int offset2;
    unsigned int ignore;
} __attribute__ ((packed)) IdtDescEntry;

typedef struct {
    unsigned short int limit;
    unsigned long long offset;
} __attribute__ ((packed)) Idtr;

void InitializeIdt();