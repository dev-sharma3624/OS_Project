#pragma once

#include <typedefs.h>

typedef struct heap_segment_header_t {
    size_t length;
    struct heap_segment_header_t* next;
    struct heap_segment_header_t* last;
    bool is_free;
    void* combined;
} heap_segment_header_t;

size_t get_heap_size();
void heap_init();
void* heap_kmalloc(size_t size);
void heap_kfree(void* address);
uint64_t get_heap_end_address();