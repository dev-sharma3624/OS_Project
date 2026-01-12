#include <typedefs.h>
#include <memory_management/paging.h>
#include <memory_management/pmm.h>
#include <memory_management/memory.h>
#include <architecture/x86_64/io.h>

#define L4_IDX(v) ((v >> 39) & 0x1FF)
#define L3_IDX(v) ((v >> 30) & 0x1FF)
#define L2_IDX(v) ((v >> 21) & 0x1FF)
#define L1_IDX(v) ((v >> 12) & 0x1FF)

paging_page_table_t* kernel_pml4 = NULL;

void paging_set_new_page_table(paging_page_table_t* page_table, uint64_t index, uint64_t flags){

    void* new_page_table_addr = pmm_request_page();
    void* new_page_table = (void*)P2V(new_page_table_addr);

    if(new_page_table == NULL){
        return;
    }

    memset(new_page_table, 0, 4096);

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
    uint64_t entry_address = entry->address << 12;
    return (paging_page_table_t*) P2V(entry_address);
}

void paging_map_page(paging_page_table_t* pml4, void* virtual_addr_ptr, uint64_t physical_addr, uint64_t flags){

    uint64_t virtual_address_value = (uint64_t) virtual_addr_ptr;

    uint64_t level_4_index = L4_IDX(virtual_address_value);

    if(!pml4->entries[level_4_index].present){
        paging_set_new_page_table(pml4, level_4_index, 7);
    }

    if(flags & PT_FLAG_READ_WRITE) {
        pml4->entries[level_4_index].read_write = 1;
    }

    paging_page_table_t* pdp = get_table_ptr(&(pml4->entries[level_4_index]));
    uint64_t level_3_index = L3_IDX(virtual_address_value);

    if(!pdp->entries[level_3_index].present){
        paging_set_new_page_table(pdp, level_3_index, 7);
    }

    if(flags & PT_FLAG_READ_WRITE) {
        pdp->entries[level_3_index].read_write = 1;
    }

    paging_page_table_t* pd = get_table_ptr(&(pdp->entries[level_3_index]));
    uint64_t level_2_index = L2_IDX(virtual_address_value);

    if(!pd->entries[level_2_index].present){
        paging_set_new_page_table(pd, level_2_index, 7);
    }

    if(flags & PT_FLAG_READ_WRITE) {
        pd->entries[level_2_index].read_write = 1;
    }

    paging_page_table_t* pt = get_table_ptr(&(pd->entries[level_2_index]));
    uint64_t level_1_index = L1_IDX(virtual_address_value);

    pt->entries[level_1_index].address = physical_addr >> 12;

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

void kernel_high_entry() {
    io_print("\nSuccessfully jumped to Higher Half!\n");

    uint64_t rsp_val;
    asm volatile ("mov %%rsp, %0" : "=r"(rsp_val));
    io_print("Current RSP: ");
    print_address_hex((void*)rsp_val);

    // ... previous print code ...

    io_print("Unmapping Lower Half (Identity Map)...\n");

    // 1. Unmap the first PML4 entry
    // This single entry covers the entire first 512GB of Virtual Memory (0x0000... to 0x0080...)
    // Since we zeroed the table earlier, this removes the Identity Map we created.
    kernel_pml4->entries[0] = (paging_map_entry_t){0}; 

    // 2. Flush the TLB (Translation Lookaside Buffer)
    // The CPU caches the old mappings. We must force it to forget them.
    // Reloading CR3 is the easiest way to flush everything.
    uint64_t cr3_val;
    asm volatile ("mov %%cr3, %0" : "=r"(cr3_val));
    asm volatile ("mov %0, %%cr3" :: "r"(cr3_val));

    io_print("Lower Half Unmapped. Testing...\n");

    // 3. The Test: Try to access a low physical address directly
    // This SHOULD crash (Page Fault) if we are successful.
    // uint64_t *bad_ptr = (uint64_t *)0x100000; 
    // *bad_ptr = 0xDEADBEEF;

    io_print("We are officially a Higher Half Kernel.\n");

    unlock_pages((void*) 0, KERNEL_PHSY_BASE/4096);
    lock_pages((void*)0, 256);
}

void paging_init(void* frame_buffer_addr, uint64_t frame_buffer_size, uint64_t total_ram_size){
    io_print("requesting page\n");

    uint64_t pml4_phsy = (uint64_t)(pmm_request_page());
    kernel_pml4 = (paging_page_table_t*) P2V(pml4_phsy);

    io_print("before memset in paging init\n");

    memset(kernel_pml4, 0, 4096);

    io_print("after memset in paging init\n");

    uint64_t frame_buffer_end = (uint64_t) frame_buffer_addr + frame_buffer_size;
    uint64_t memory_end = total_ram_size;

    if(frame_buffer_end > memory_end){
        memory_end = frame_buffer_end;
    }

    if(memory_end % 4096 != 0){
        memory_end = (memory_end & ~0xFFF) + 4096;
    }

    for(uint64_t addr = 0; addr < memory_end ; addr += 4096){
        /* paging_map_page(
            kernel_pml4,
            (void*) addr,
            addr,
            PT_FLAG_PRESENT | PT_FLAG_READ_WRITE
        ); */

        paging_map_page(
            kernel_pml4,
            (void*)P2V(addr),
            addr,
            PT_FLAG_PRESENT | PT_FLAG_READ_WRITE
        );
    }

    /* uint64_t fb_phys_start = (uint64_t)frame_buffer_addr;
    uint64_t fb_phys_end = fb_phys_start + frame_buffer_size;

    // Align End
    if(fb_phys_end % 4096 != 0){
        fb_phys_end = (fb_phys_end & ~0xFFF) + 4096;
    }

    uint64_t fb_virt_start = FRAMEBUFFER_VIRT_ADDR;
    
    for(uint64_t i = 0; i < frame_buffer_size; i += 4096){
        uint64_t phys = fb_phys_start + i;
        uint64_t virt = fb_virt_start + i;
        
        // Map Explicit Virtual -> Explicit Physical
        paging_map_page(kernel_pml4, (void*)virt, phys, PT_FLAG_PRESENT | PT_FLAG_READ_WRITE | PT_FLAG_CACHE_DISABLED); // Keep PCD bit!
    }
 */
    load_cr3((void*) pml4_phsy);

    unlock_pages((void*) 0, KERNEL_PHSY_BASE/4096);
    lock_pages((void*)0, 256);

    /* uint64_t target_addr = (uint64_t)&kernel_high_entry;

    // 2. Ensure it is a Higher Half address
    // If your linker script is already set to 0xFF..., target_addr might already be high.
    // If it is low (0x80...), we force it high using your P2V logic.
    if (target_addr < 0xFFFFFFFF00000000) {
        target_addr = (uint64_t)P2V(target_addr);
    }

    // 3. Create a function pointer to the High Address
    void (*high_func)() = (void (*)())target_addr;

    io_print("Jumping to Higher Half...\n");

    // 4. The Leap
    high_func(); */

}