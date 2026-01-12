#include <typedefs.h>
#include "../../boot_info.h"
#include <memory_management/memory.h>
#include <memory_management/m_bitmap.h>
#include <memory_management/m_desc.h>

static memory_bitmap bitmap;
extern uint64_t _KernelStart;
extern uint64_t _KernelEnd;
uint64_t free_memory;
uint64_t reserved_memory;
uint64_t used_memory;
bool initialized = false;


#define PAGE_SIZE 4096
#define RESERVE_MEMORY() (free_memory -= PAGE_SIZE,  reserved_memory += PAGE_SIZE)
#define UNRESERVE_MEMORY() (free_memory += PAGE_SIZE, reserved_memory -= PAGE_SIZE)
#define PAGE_ALLOCATE() (used_memory += PAGE_SIZE, free_memory -= PAGE_SIZE)
#define PAGE_FREE() (used_memory -= PAGE_SIZE, free_memory += PAGE_SIZE)


static void lock_page(void* address){
    uint64_t index = (uint64_t) address / 4096;
    if(m_bitmap_get_set_memory_bit(&bitmap, index, true)){
        RESERVE_MEMORY();
    }
}

void lock_pages(void* address, uint64_t count){
    for(uint64_t i = 0; i < count; i++){
        void* addr = (void*)((uint64_t)address + (i * 4096));
        if(i == 0 || i == count - 1){
            uint64_t index = (uint64_t) addr / 4096;
        }
        lock_page(addr);
    }
}


static void unlock_page(void* address){
    uint64_t index = (uint64_t) address / 4096;
    if(m_bitmap_get_set_memory_bit(&bitmap, index, false)){
        UNRESERVE_MEMORY();
    }
}

void unlock_pages(void* address, uint64_t count){
    for(uint64_t i = 0; i < count; i++){
        void* addr = (void*)((uint64_t)address + (i * 4096));
        if(i == 0 || i == count - 1){
            uint64_t index = (uint64_t) addr / 4096;
        }
        unlock_page(addr);
    }
}


void pmm_init(boot_info_t* boot_info){

    if(initialized){
        return;
    }
    initialized = true;

    uint64_t memory_size = memory_get_m_size(boot_info);
    uint64_t total_pages = memory_size / 4096;
    uint64_t bitmap_size = (total_pages / 8) + 1;
    uint64_t m_map_entries = boot_info->m_map_size / boot_info->m_map_desc_size;

    reserved_memory = memory_size;
    free_memory = 0;
    used_memory = 0;

    void* bitmap_location = (void*)P2V(memory_find_suitable_m_segment(boot_info, bitmap_size));

    if (bitmap_location == NULL) {
        while(1) asm("hlt");
    }

    m_bitmap_init(&bitmap, bitmap_size, (uint8_t*)bitmap_location);

    for(uint64_t i = 0; i < m_map_entries; i++){
        memory_descriptor_t* desc = (memory_descriptor_t*)((uint64_t)boot_info->m_map + (i * boot_info->m_map_desc_size));

        if(desc->type == 7){
            unlock_pages((void*)desc->physical_start, desc->number_of_pages);
        }
    }

    uint64_t pages_required_for_bitmap = (bitmap.size / 4096) + 1;
    void* bitmap_start_address = (void*)V2P(bitmap.address);
    lock_pages(bitmap_start_address, pages_required_for_bitmap);

    uint64_t kernel_start = V2P((uint64_t)&_KernelStart);
    uint64_t kernel_end = V2P((uint64_t)&_KernelEnd);
    uint64_t kernel_size = kernel_end - kernel_start;
    uint64_t pages_required_for_kernel = (kernel_size / 4096) + 1;
    lock_pages((void*)kernel_start, pages_required_for_kernel);

    uint64_t pages_to_lock = KERNEL_PHSY_BASE / 4096;
    lock_pages((void*)0, pages_to_lock);
}

void* pmm_request_page(){
    uint64_t first_free_index = m_bitmap_get_first_free_memory_bit(&bitmap);

    bool result = m_bitmap_get_set_memory_bit(&bitmap, first_free_index, true);

    if(result){
        uint64_t address = first_free_index * 4096;

        PAGE_ALLOCATE();

        return (void*) address;
    }
}

void pmm_free_page(void* address){

    uint64_t index = ((uint64_t) address) / 4096;

    bool result = m_bitmap_get_set_memory_bit(&bitmap, index, false);
    
    PAGE_FREE();

}