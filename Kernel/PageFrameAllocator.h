#pragma once

#include "Typedefs.h"
#include "../boot_info.h"

void InitPageFrameAllocator(BOOT_INFO* bootInfo);

void* RequestPage();

void FreePage(void* address);