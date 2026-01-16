#pragma once
#include <typedefs.h>
#include <architecture/x86_64/trap_frame.h>
#include <memory_management/paging.h>

// different states that a process can be in
typedef enum task_state_t {
    TASK_READY,
    TASK_RUNNING,
    TASK_BLOCKED,
    TASK_ZOMBIE
} task_state_t;

// represents a process/thread
typedef struct tcb_t {
    uint64_t rsp; // the stored stack pointer when task is not running
    uint64_t cr3; // physical address of the page map

    uint64_t pid; // process id
    task_state_t task_state; // current state
    uint64_t wakeup_time;

    char name[16]; // task name (for debugging)

    void* stack_base; // pointer to the base of allocated stack (for freeing)
    uint64_t heap_end;
    paging_page_table_t* pml4;
    struct tcb_t* next; // pointer to next tcb in linked list
} tcb_t;

void multitask_init();
void create_task(void (*entry_point) (void)); // parameter -> pointer to a function that returns void
void create_user_task(void (*entry_point)(void));