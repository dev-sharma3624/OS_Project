#pragma once

#include "typedefs.h"

typedef struct {
    uint64_t size;
    uint8_t* address;
} memory_bitmap;

void m_bitmap_init(memory_bitmap* memory_bitmap, uint64_t size, uint8_t* address);

bool m_bitmap_get_memory_bit(memory_bitmap* memory_bitmap, uint64_t index);

bool m_bitmap_get_set_memory_bit(memory_bitmap* memory_bitmap, uint64_t index, bool value);

uint64_t m_bitmap_get_first_free_memory_bit(memory_bitmap* memory_bitmap);