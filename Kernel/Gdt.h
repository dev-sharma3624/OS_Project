#pragma once


typedef struct {
    unsigned short int limit0;
    unsigned short int base0;
    unsigned char base1;
    unsigned char accessByte;
    unsigned char limit1Flags;
    unsigned char base2;
} GdtEntry;

typedef struct {
    unsigned short int limit;
    unsigned long long int offset;
} __attribute__((packed)) GdtDescriptor;

typedef struct {
    GdtEntry null;
    GdtEntry kernelCode;
    GdtEntry kernelData;
    GdtEntry userNull;
    GdtEntry userCode;
    GdtEntry userData;
} Gdt;

void InitializeGDT();
void LoadGDT(GdtDescriptor* gdtDescriptor);
