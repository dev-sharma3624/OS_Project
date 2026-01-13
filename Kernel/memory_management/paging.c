#include <typedefs.h>
#include <memory_management/paging.h>
#include <memory_management/pmm.h>
#include <memory_management/memory.h>
#include <architecture/x86_64/io.h>

#define L4_IDX(v) ((v >> 39) & 0x1FF) //9 bits, from 39-47
#define L3_IDX(v) ((v >> 30) & 0x1FF) //9 bits, from 30-38
#define L2_IDX(v) ((v >> 21) & 0x1FF) //9 bits, from 21-29
#define L1_IDX(v) ((v >> 12) & 0x1FF) //9 bits, from 12-20
//12 bits, from 0-11 is offset within the physical 4kb page

paging_page_table_t* kernel_pml4 = NULL;

paging_page_table_t* get_kernel_page_table(){
    return kernel_pml4;
}

void paging_set_new_page_table(paging_page_table_t* page_table, uint64_t index, uint64_t flags){

    void* new_page_table_addr = pmm_request_page(); //physical address to store new table
    void* new_page_table = (void*)P2V(new_page_table_addr); //corresponding virtual address

    if(new_page_table == NULL){ //overflow check
        return;
    }

    memset(new_page_table, 0, 4096); //clearing the page

    //setting up bit values
    page_table->entries[index].address = (uint64_t) new_page_table_addr >> 12;
    page_table->entries[index].present = (flags & PT_FLAG_PRESENT) ? 1 : 0;
    page_table->entries[index].read_write = (flags & PT_FLAG_READ_WRITE) ? 1 : 0;
    page_table->entries[index].user_super = (flags & PT_FLAG_USER_SUPER) ? 1 : 0;
    page_table->entries[index].write_through = (flags & PT_FLAG_WRITE_THROUGH) ? 1 : 0;
    page_table->entries[index].cache_disabled = (flags & PT_FLAG_CACHE_DISABLED) ? 1 : 0;
    page_table->entries[index].accessed = (flags & PT_FLAG_ACCESSED) ? 1 : 0;
    page_table->entries[index].dirty = (flags & PT_FLAG_DIRTY) ? 1 : 0;
    page_table->entries[index].huge_page = (flags & PT_FLAG_HUGE_PAGE) ? 1 : 0;
    page_table->entries[index].global = (flags & PT_FLAG_GLOBAL) ? 1 : 0;
    page_table->entries[index].nx = (flags & PT_FLAG_NX) ? 1 : 0;

}

paging_page_table_t* get_table_ptr(paging_map_entry_t* entry){

    //address field is 40 bits, after left-shift entry_address gets 52 bits
    //with left most 12 bits as 0 which is important since pages are 4kb aligned
    uint64_t entry_address = entry->address << 12;
    return (paging_page_table_t*) P2V(entry_address);
}

void paging_map_page(paging_page_table_t* pml4, void* virtual_addr_ptr, uint64_t physical_addr, uint64_t flags){

    uint64_t virtual_address_value = (uint64_t) virtual_addr_ptr;

    uint64_t level_4_index = L4_IDX(virtual_address_value);
    uint64_t level_3_index = L3_IDX(virtual_address_value);
    uint64_t level_2_index = L2_IDX(virtual_address_value);
    uint64_t level_1_index = L1_IDX(virtual_address_value);




    //----------------------------PML4 (ROOT LEVEL)--------------------------------------------------

    if(!pml4->entries[level_4_index].present){ //setting new page table if not present
        paging_set_new_page_table(pml4, level_4_index, PT_FLAG_PRESENT | PT_FLAG_READ_WRITE | PT_FLAG_USER_SUPER);
    }

    paging_map_entry_t pml4_entry = pml4->entries[level_4_index]; //getting pdp entry




    //------------------------PDPT (PAGE DIRECTOR POINTER TABLE)------------------------------------

    paging_page_table_t* pdp = get_table_ptr(&pml4_entry); //going to address provided by the above entry

    if(!pdp->entries[level_3_index].present){
        paging_set_new_page_table(pdp, level_3_index, PT_FLAG_PRESENT | PT_FLAG_READ_WRITE | PT_FLAG_USER_SUPER);
    }




   	//-------------------------PAGE DIRECTORY--------------------------------------------------------------


    paging_page_table_t* pd = get_table_ptr(&(pdp->entries[level_3_index]));

    if(!pd->entries[level_2_index].present){
        paging_set_new_page_table(pd, level_2_index, PT_FLAG_PRESENT | PT_FLAG_READ_WRITE | PT_FLAG_USER_SUPER);
    }




   	//------------------------------PAGE TABLE------------------------------------------------------

    paging_page_table_t* pt = get_table_ptr(&(pd->entries[level_2_index]));

    //offset
    pt->entries[level_1_index].address = physical_addr >> 12;

    //flags
    pt->entries[level_1_index].present = (flags & PT_FLAG_PRESENT) ? 1 : 0;
    pt->entries[level_1_index].read_write = (flags & PT_FLAG_READ_WRITE) ? 1 : 0;
    pt->entries[level_1_index].user_super = (flags & PT_FLAG_USER_SUPER) ? 1 : 0;
    pt->entries[level_1_index].write_through = (flags & PT_FLAG_WRITE_THROUGH) ? 1 : 0;
    pt->entries[level_1_index].cache_disabled = (flags & PT_FLAG_CACHE_DISABLED) ? 1 : 0;
    pt->entries[level_1_index].accessed = (flags & PT_FLAG_ACCESSED) ? 1 : 0;
    pt->entries[level_1_index].dirty = (flags & PT_FLAG_DIRTY) ? 1 : 0;
    pt->entries[level_1_index].huge_page = (flags & PT_FLAG_HUGE_PAGE) ? 1 : 0;
    pt->entries[level_1_index].global = (flags & PT_FLAG_GLOBAL) ? 1 : 0;
    pt->entries[level_1_index].nx = (flags & PT_FLAG_NX) ? 1 : 0;

}

void load_cr3(void* cr3_value) {
    asm volatile("mov %0, %%cr3" :: "r" (cr3_value) : "memory");
}

void paging_init(void* frame_buffer_addr, uint64_t frame_buffer_size, uint64_t total_ram_size){

    //requesting a 4kb page from pmm, converting it to it's corresponding virtual address
    uint64_t pml4_phsy = (uint64_t)(pmm_request_page());
    kernel_pml4 = (paging_page_table_t*) P2V(pml4_phsy);

    memset(kernel_pml4, 0, 4096); //clearing the page

    uint64_t frame_buffer_end = (uint64_t) frame_buffer_addr + frame_buffer_size;

    uint64_t memory_end = total_ram_size;

    //updating memory end to frame_buffer_end in case it exceeds in order to cover whole range of memory
    if(frame_buffer_end > memory_end){
        memory_end = frame_buffer_end;
    }

    // aligningn memory to 4096 bytes (12 rightmost bits must be zero)
    if(memory_end % 4096 != 0){
        memory_end = (memory_end & ~0xFFF) + 4096; //clearing 12 rightmost bits, adding padding to cover anything that was discarded due to clearing
    }

    for(uint64_t addr = 0; addr < memory_end ; addr += 4096){ //mapping every 4kb page (base address) to it's corresponding virtual address

        paging_map_page(
            kernel_pml4,                            //page table
            (void*)P2V(addr),                       //virtual address
            addr,                                   //phsyical address
            PT_FLAG_PRESENT | PT_FLAG_READ_WRITE    //flags
        );
        
    }

    load_cr3((void*) pml4_phsy); //loading new page table

    //unlocking all pages below the kernel physical base address that were locked
    //to avoid page faults that might trigger since page tables created during kernel jump
    //might not have mapped it
    unlock_pages((void*) 0, KERNEL_PHSY_BASE/4096);

    //locking lower 1mb
    lock_pages((void*)0, 256);

}