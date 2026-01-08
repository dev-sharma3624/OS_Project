#pragma once
#include <typedefs.h>

static inline __attribute__((always_inline)) int get_intrpt_flag(){
    size_t rflags;
    asm volatile("pushfq; pop %0" : "=r"(rflags));
    return (rflags & 0x200)? 1 : 0;
}

void push_cli();
void pop_cli();