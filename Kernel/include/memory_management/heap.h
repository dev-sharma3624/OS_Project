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
void heap_init(void* heap_start, size_t heap_size);
void* heap_kmalloc(size_t size);
void heap_kfree(void* address);