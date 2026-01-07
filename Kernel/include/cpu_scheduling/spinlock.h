#pragma once
#include <typedefs.h>
#include <architecture/x86_64/spinlock_atomic_instr.h>

typedef struct {
    volatile uint64_t locked;
} spinlock_t;

static inline void spinlock_init(spinlock_t* lock) {
    lock->locked = 0;
}


static inline __attribute__((always_inline)) void spinlock_acquire(spinlock_t* lock) {

    while (1) {

        if (atomic_xchg(&lock->locked, 1) == 0) {
            return;
        }

        while (lock->locked == 1) {
            cpu_relax(); 
        }
    }
}

static inline __attribute__((always_inline)) void spinlock_release(spinlock_t* lock) {
    atomic_xchg(&lock->locked, 0);
}