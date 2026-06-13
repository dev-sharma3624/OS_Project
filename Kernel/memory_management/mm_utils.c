#include "../../boot_info.h"
#include <typedefs.h>
#include <memory_management/m_desc.h>
#include "mm_utils.h"

enum ITERATOR_RETURN_CODE{
    SUCCESS,
    ERROR,
    KILL_LOOP,
    CONTINUE
};

void mm_utils_iterator(boot_info_t* boot_info, uint64_t* dependency_ptr, int (*operation)(memory_descriptor_t* desc, uint64_t* dependency_ptr)){

    uint64_t m_map_entries = boot_info->m_map_size / boot_info->m_map_desc_size;
    uint64_t m_map_desc_size = boot_info->m_map_desc_size;

    for(uint64_t i = 0; i < m_map_entries; i++){

        memory_descriptor_t* desc = (memory_descriptor_t*) ((uint64_t)boot_info->m_map + (i * m_map_desc_size));

        if(desc->type == EFI_CONVENTIONAL_MEMORY){

            int result =  operation(desc, dependency_ptr);

            if(result == KILL_LOOP){
                break;
            }

        }

    }

}

int mm_utils_calc_m_size(memory_descriptor_t* desc, uint64_t* result){

    uint64_t block_size = desc->number_of_pages * 4096;

    *result += block_size;

    return SUCCESS;

}

int mm_utils_find_segment(memory_descriptor_t* desc, uint64_t* context){

    //first element is required segment size, second is the phsyical start address that we want
    //uint64_t context[2] = { mininmum_segment_size, 0 };

    if (context[1] != 0) {
        return KILL_LOOP; 
    }

    if (desc->physical_start < KERNEL_PHSY_BASE) {
        return CONTINUE;
    }

    uint64_t segment_size = desc->number_of_pages * 4096;
    
    if (segment_size > context[0]){

        context[1] = desc->physical_start; 
        
        return KILL_LOOP; 
    }

    return CONTINUE;
}