#pragma once

#include "Typedefs.h"

typedef struct {
    uint64_t size;
    uint8_t* address;
} MemoryBitmap;

void InitMemoryBitmap(MemoryBitmap* memoryBitmap, uint64_t size, uint8_t* address);

bool GetMemoryBit(MemoryBitmap* memoryBitmap, uint64_t index);

bool SetMemoryBit(MemoryBitmap* memoryBitmap, uint64_t index, bool value);

uint64_t GetFirstFreeMemoryBit(MemoryBitmap* memoryBitmap);