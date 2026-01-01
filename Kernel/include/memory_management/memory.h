#pragma once

#include <typedefs.h>
#include "../../boot_info.h"

uint64_t memory_get_m_size(boot_info_t* boot_info);
uint64_t memory_find_suitable_m_segment(boot_info_t* boot_info, uint64_t mininmum_segment_size);