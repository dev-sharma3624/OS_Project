#pragma once
#include <typedefs.h>

static inline __attribute__((always_inline)) uint64_t atomic_xchg(volatile uint64_t* ptr, uint64_t new_val) {

    uint64_t result = new_val;
    
    asm volatile(
        "xchg %0, %1"          
        : "+r"(result),         
          "+m"(*ptr)            
        :                       
        : "memory"              
    );

    return result;
}

static inline __attribute__((always_inline)) void cpu_relax() {
    asm volatile("pause" ::: "memory");
}