#include <typedefs.h>
#include <memory_management/paging.h>
#include <memory_management/pmm.h>
#include <memory_management/memory.h>

#define L4_IDX(v) ((v >> 39) & 0x1FF)
#define L3_IDX(v) ((v >> 30) & 0x1FF)
#define L2_IDX(v) ((v >> 21) & 0x1FF)
#define L1_IDX(v) ((v >> 12) & 0x1FF)

paging_page_table_t* kernel_pml4 = NULL;

void paging_set_new_page_table(paging_page_table_t* page_table, uint64_t index, uint64_t flags){

    void* new_page_table = pmm_request_page();

    if(new_page_table == NULL){
        return;
    }

    memset(new_page_table, 0, 512);

    page_table->entries[index].address = (uint64_t) new_page_table >> 12;
    page_table->entries[index].present = (flags & 0x01) ? 1 : 0;
    page_table->entries[index].read_write = (flags & 0x02) ? 1 : 0;
    page_table->entries[index].user_super = (flags & 0x04) ? 1 : 0;

}

paging_page_table_t* get_table_ptr(paging_map_entry_t* entry){
    uint64_t entry_address = entry->address << 12;
    return (paging_page_table_t*) entry_address;
}

void paging_map_page(paging_page_table_t* pml4, void* virtual_addr_ptr, void* physical_addr, uint64_t flags){

    uint64_t virtual_address_value = (uint64_t) virtual_addr_ptr;

    uint64_t level_4_index = L4_IDX(virtual_address_value);

    if(!pml4->entries[level_4_index].present){
        paging_set_new_page_table(pml4, level_4_index, 7);
    }

    paging_page_table_t* pdp = get_table_ptr(&(pml4->entries[level_4_index]));
    uint64_t level_3_index = L3_IDX(virtual_address_value);

    if(!pdp->entries[level_3_index].present){
        paging_set_new_page_table(pdp, level_3_index, 7);
    }

    paging_page_table_t* pd = get_table_ptr(&(pdp->entries[level_3_index]));
    uint64_t level_2_index = L2_IDX(virtual_address_value);

    if(!pd->entries[level_2_index].present){
        paging_set_new_page_table(pd, level_2_index, 7);
    }

    paging_page_table_t* pt = get_table_ptr(&(pd->entries[level_2_index]));
    uint64_t level_1_index = L1_IDX(virtual_address_value);

    pt->entries[level_1_index].address = (uint64_t) physical_addr >> 12;

    pt->entries[level_1_index].present = (flags & PT_FLAG_PRESENT) ? 1 : 0;
    pt->entries[level_1_index].read_write = (flags & PT_FLAG_READ_WRITE) ? 1 : 0;
    pt->entries[level_1_index].user_super = (flags & PT_FLAG_USER_SUPER) ? 1 : 0;

}

void load_cr3(void* cr3_value) {
    asm volatile("mov %0, %%cr3" :: "r" (cr3_value) : "memory");
}

void paging_init(void* frame_buffer_addr, uint64_t frame_buffer_size, uint64_t total_ram_size){

    kernel_pml4 = (paging_page_table_t*) pmm_request_page();

    memset(kernel_pml4, 0, 512);

    uint64_t frame_buffer_end = (uint64_t) frame_buffer_addr + frame_buffer_size;
    uint64_t memory_end = total_ram_size;

    if(frame_buffer_end > memory_end){
        memory_end = frame_buffer_end;
    }

    if(memory_end % 4096 != 0){
        memory_end = (memory_end & ~0xFFF) + 4096;
    }

    for(uint64_t addr = 0; addr < memory_end ; addr += 4096){
        paging_map_page(
            kernel_pml4,
            (void*) addr,
            (void*) addr,
            PT_FLAG_PRESENT | PT_FLAG_READ_WRITE
        );
    }

    load_cr3((void*) kernel_pml4);

}