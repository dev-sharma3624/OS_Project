#include <memory_management/memory.h>
#include <typedefs.h>
#include "../../boot_info.h"
#include <memory_management/m_desc.h>
#include "mm_utils.h"

uint64_t memory_get_m_size(boot_info_t* boot_info){
    
    uint64_t total_memory = 0;

    mm_utils_iterator(boot_info, &total_memory, mm_utils_calc_m_size);

    return total_memory;

}

uint64_t memory_find_suitable_m_segment(boot_info_t* boot_info, uint64_t mininmum_segment_size){

    //first element is required segment size, second is the phsyical start address that we want
    uint64_t context[2] = { mininmum_segment_size, 0 };
    mm_utils_iterator(boot_info, context, mm_utils_find_segment);

    if(context[1] != 0){
        return context[1];
    }

    return NULL;
}

void memset(void* start_address, uint64_t value, size_t limit){

    uint8_t* start_address_uint8t_pointer = (uint8_t*) start_address;

    for(uint64_t i = 0; i < limit; i++){
        start_address_uint8t_pointer[i] = value;
    }
}


void* memcpy(void* dest, const void* src, size_t n) {
    char* d = (char*)dest;
    const char* s = (const char*)src;

    for (size_t i = 0; i < n; i++) {
        d[i] = s[i];
    }

    return dest;
}