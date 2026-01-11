#include <memory_management/memory.h>
#include <typedefs.h>
#include "../../boot_info.h"
#include <memory_management/m_desc.h>

uint64_t memory_get_m_size(boot_info_t* boot_info){

    uint64_t total_memory = 0;
    
    uint64_t m_map_enntries = boot_info->m_map_size / boot_info->m_map_desc_size;
    uint64_t m_map_desc_size = boot_info->m_map_desc_size;

    for(uint64_t i = 0; i < m_map_enntries; i++){

        memory_descriptor_t* desc = (memory_descriptor_t*) ((uint64_t)boot_info->m_map + (i * m_map_desc_size));

        if(desc->type == 7){

            uint64_t physical_end = desc->physical_start + (desc->number_of_pages * 4096);

            if(physical_end > total_memory){
                total_memory = physical_end;
            }

        }

    }

    return total_memory;

}

uint64_t memory_find_suitable_m_segment(boot_info_t* boot_info, uint64_t mininmum_segment_size){
    uint64_t m_map_enntries = boot_info->m_map_size / boot_info->m_map_desc_size;

    for(uint64_t i = 0; i < m_map_enntries; i++){

        memory_descriptor_t* desc = (memory_descriptor_t*) ((uint64_t)boot_info->m_map + (i * boot_info->m_map_desc_size));

        if (desc->physical_start < KERNEL_PHSY_BASE) continue;

        if(desc->type == 7){

            uint64_t segment_size = desc->number_of_pages * 4096;
            if(segment_size > mininmum_segment_size){
                return desc->physical_start;
            }

        }

    }

    return NULL;
}

void memset(void* start_address, uint64_t value, size_t limit){

    uint8_t* start_address_uint8t_pointer = (uint8_t*) start_address;

    for(uint64_t i = 0; i < limit; i++){
        start_address_uint8t_pointer[i] = value;
    }
}