#pragma once

#include "Typedefs.h"

typedef struct{
    uint32_t type;
    uint64_t physicalStart;
    uint64_t virtualStart;
    uint64_t numberOfPages;
    uint64_t attribute;
} MemoryDescriptor;