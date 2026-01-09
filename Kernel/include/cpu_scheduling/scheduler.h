#pragma once
#include <typedefs.h>

void schedule();
void task_yield();
void task_sleep(uint64_t time_in_ms);
void task_check_wakeup();