#pragma once

#include "Typedefs.h"
#include "../boot_info.h"


extern uint64_t freeMemory;
extern uint64_t reservedMemory;
extern uint64_t usedMemory;
extern bool initialized;

void InitPageFrameAllocator(BOOT_INFO* bootInfo);

void* RequestPage();

void FreePage(void* address);