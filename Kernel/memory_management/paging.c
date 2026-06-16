#include "../../boot_info.h"
#include <typedefs.h>
#include <memory_management/paging.h>
#include <memory_management/pmm.h>
#include <memory_management/memory.h>
#include "mm_utils.h"

#define L4_IDX(v) ((v >> 39) & 0x1FF) //9 bits, from 39-47
#define L3_IDX(v) ((v >> 30) & 0x1FF) //9 bits, from 30-38
#define L2_IDX(v) ((v >> 21) & 0x1FF) //9 bits, from 21-29
#define L1_IDX(v) ((v >> 12) & 0x1FF) //9 bits, from 12-20
//12 bits, from 0-11 is offset within the physical 4kb page

extern uint64_t _KernelStart;
extern uint64_t _KernelEnd;

paging_page_table_t* kernel_pml4 = NULL;
bool cr3_loaded = false;

paging_page_table_t* get_kernel_page_table(){
    return kernel_pml4;
}

void paging_set_flags(paging_page_table_t* page_table, uint64_t index, uint64_t flags, uint64_t phy_addr){
    
    page_table->entries[index].address = (uint64_t) phy_addr >> 12;
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

void paging_set_new_page_table(paging_page_table_t* page_table, uint64_t index, uint64_t flags){

    void* new_page_table_addr = pmm_request_page(); //physical address to store new table
    void* new_page_table = NULL; //corresponding virtual address

    if(!cr3_loaded){
        memset( (void*) P2V(new_page_table_addr), 0, 4096);
        new_page_table = (void*) P2V((uint64_t)new_page_table_addr);
    }else{    
        memset( (void*) P2V_DIRECT(new_page_table_addr), 0, 4096);
        new_page_table = (void*) P2V_DIRECT((uint64_t)new_page_table_addr);
    }

    if(new_page_table == NULL){ //overflow check
        return;
    }

    //setting up bit values
    paging_set_flags(page_table, index, flags, (uint64_t) new_page_table_addr);

}

paging_page_table_t* get_table_ptr(paging_map_entry_t* entry){

    //address field is 40 bits, after left-shift entry_address gets 52 bits
    //with left most 12 bits as 0 which is important since pages are 4kb aligned
    uint64_t entry_address = entry->address << 12;

    if(!cr3_loaded){
        print_address_hex(P2V(entry_address));
        return (paging_page_table_t*) P2V(entry_address);
    }else{
        print_address_hex(P2V_DIRECT(entry_address));
        return (paging_page_table_t*) P2V_DIRECT(entry_address);
    }

}

void paging_map_page(paging_page_table_t* pml4, void* virtual_addr_ptr, uint64_t physical_addr, uint64_t flags, PAGE_SIZE page_size){

    uint64_t virtual_address_value = (uint64_t) virtual_addr_ptr;
    uint64_t level_4_index = L4_IDX(virtual_address_value);
    uint64_t level_3_index = L3_IDX(virtual_address_value);
    uint64_t level_2_index = L2_IDX(virtual_address_value);
    uint64_t level_1_index = L1_IDX(virtual_address_value);

    //----------------------------PML4 (ROOT LEVEL)--------------------------------------------------

    if(!pml4->entries[level_4_index].present){ //setting new page table if not present
        paging_set_new_page_table(pml4, level_4_index, PT_FLAG_PRESENT | PT_FLAG_READ_WRITE);
    }
    paging_map_entry_t pml4_entry = pml4->entries[level_4_index]; //getting pdp entry

    //------------------------PDPT (PAGE DIRECTOR POINTER TABLE)------------------------------------

    paging_page_table_t* pdp = get_table_ptr(&pml4_entry); //going to address provided by the above entry
    if(!pdp->entries[level_3_index].present){
        paging_set_new_page_table(pdp, level_3_index, PT_FLAG_PRESENT | PT_FLAG_READ_WRITE);
    }

   	//-------------------------PAGE DIRECTORY--------------------------------------------------------------

    paging_page_table_t* pd = get_table_ptr(&(pdp->entries[level_3_index]));

    if(page_size == MB_2){
        paging_set_flags(pd, level_2_index, flags, physical_addr);
        return;
    }

   	//------------------------------PAGE TABLE------------------------------------------------------

    if(!pd->entries[level_2_index].present){
        paging_set_new_page_table(pd, level_2_index, PT_FLAG_PRESENT | PT_FLAG_READ_WRITE);
    }
    paging_page_table_t* pt = get_table_ptr(&(pd->entries[level_2_index]));
    paging_set_flags(pt, level_1_index, flags, physical_addr);

}

void load_cr3(void* cr3_value) {
    asm volatile("mov %0, %%cr3" :: "r" (cr3_value) : "memory");
}

void paging_init(boot_info_t* boot_info){

    //requesting a 4kb page from pmm, converting it to it's corresponding virtual address
    uint64_t pml4_phsy = (uint64_t)(pmm_request_page());
    kernel_pml4 = (paging_page_table_t*) P2V(pml4_phsy);

    paging_map_page(
        kernel_pml4,                                                //page table
        (void*) P2V(pml4_phsy),                                     //virtual address
        pml4_phsy,                                                  //phsyical address
        PT_FLAG_PRESENT | PT_FLAG_READ_WRITE,                       //flags
        KB_4
    );


    uint64_t pages_required_for_bitmap = (get_bitmap_size() / 4096) + 1;
    uint64_t bitmap_start_address = V2P(get_bitmap_address());
    for(uint64_t i = 0; i < pages_required_for_bitmap; i++){
        uint64_t addr = (uint64_t)bitmap_start_address + (i * 4096);
        paging_map_page(
            kernel_pml4,                                                //page table
            (void*)P2V(addr),                                           //virtual address
            addr,                                                       //phsyical address
            PT_FLAG_PRESENT | PT_FLAG_READ_WRITE,                       //flags
            KB_4
        );
    }

    uint64_t memory_end = memory_get_m_size(boot_info);

    for(uint64_t addr = 0; addr < memory_end ; addr += 0x200000){ //mapping every 4kb page (base address) to it's corresponding virtual address

        paging_map_page(
            kernel_pml4,                                                //page table
            (void*)P2V_DIRECT(addr),                                    //virtual address
            addr,                                                       //phsyical address
            PT_FLAG_PRESENT | PT_FLAG_READ_WRITE | PT_FLAG_HUGE_PAGE,   //flags
            MB_2
        );
        
    }


    uint64_t frame_buffer_base = (uint64_t) boot_info->frame_buffer.frame_buffer_base;
    uint64_t frame_buffer_size = boot_info->frame_buffer.frame_buffer_size;

    if(frame_buffer_base % 4096 != 0){
        frame_buffer_base = frame_buffer_base & (~0x0FFF);
    }

    for(uint64_t addr = 0; addr < frame_buffer_size; addr += 4096){
        paging_map_page(
            kernel_pml4,                                                                         //page table
            (void*)P2V(frame_buffer_base + addr),                                                //virtual address
            frame_buffer_base + addr,                                                            //phsyical address
            PT_FLAG_PRESENT | PT_FLAG_READ_WRITE | PT_FLAG_CACHE_DISABLED,                       //flags
            KB_4
        );
    }


    paging_map_page(
        kernel_pml4,                                                //page table
        (void*)P2V((uint64_t)boot_info->font->header),              //virtual address
        (uint64_t)boot_info->font->header,                          //phsyical address
        PT_FLAG_PRESENT,                                            //flags
        KB_4
    );

    uint64_t font_base = (uint64_t) boot_info->font->glyph_buffer;
    uint64_t font_size = 0;
    if(boot_info->font->header->mode == 1){
        font_size = boot_info->font->header->char_size * 512;
    }else{
        font_size = boot_info->font->header->char_size * 256;
    }

    if(font_base % 4096 != 0){
        font_base = font_base & (~0x0FFF);
    }

    for(uint64_t addr = 0; addr < font_size; addr += 0x200000){
        paging_map_page(
            kernel_pml4,                                                //page table
            (void*)P2V(font_base + addr),                               //virtual address
            font_base + addr,                                           //phsyical address
            PT_FLAG_PRESENT | PT_FLAG_HUGE_PAGE,                        //flags
            MB_2
        );
    }

    uint64_t kernel_start = V2P((uint64_t)&_KernelStart);
    uint64_t kernel_end = V2P((uint64_t)&_KernelEnd);
    uint64_t kernel_size = kernel_end - kernel_start;
    uint64_t pages_required_for_kernel = (kernel_size / 4096) + 1;
    for(uint64_t i = 0; i < pages_required_for_kernel; i++){
        uint64_t addr = (uint64_t)kernel_start + (i * 4096);
        paging_map_page(
            kernel_pml4,                                                //page table
            (void*)P2V(addr),                                           //virtual address
            addr,                                                       //phsyical address
            PT_FLAG_PRESENT | PT_FLAG_READ_WRITE,                       //flags
            KB_4
        );
    }

    load_cr3((void*) pml4_phsy); //loading new page table
    cr3_loaded = true;

    //unlocking all pages below the kernel physical base address that were locked
    //to avoid page faults that might trigger since page tables created during kernel jump
    //might not have mapped it
    unlock_pages((void*) 0, KERNEL_PHSY_BASE/4096);

    //locking lower 1mb
    lock_pages((void*)0, 256);
}