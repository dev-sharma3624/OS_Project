#pragma once

#include "../../boot_info.h"
#include <typedefs.h>
#include <memory_management/m_desc.h>

void mm_utils_iterator(
    boot_info_t* boot_info,
    uint64_t* dependency_ptr,
    int (*operation)(memory_descriptor_t* desc, uint64_t* dependency_ptr)
);

int mm_utils_calc_m_size(memory_descriptor_t* desc, uint64_t* result);

int mm_utils_find_segment(memory_descriptor_t* desc, uint64_t* context);

uint64_t get_bitmap_address();

uint64_t get_bitmap_size();