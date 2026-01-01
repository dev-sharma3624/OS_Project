#pragma once

#include "typedefs.h"
#include "../boot_info.h"


extern uint64_t free_memory;
extern uint64_t reserved_memory;
extern uint64_t used_memory;
extern bool initialized;

void pmm_init(boot_info_t* boot_info);

void* pmm_request_page();

void pmm_free_page(void* address);