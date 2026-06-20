#include <typedefs.h>
#include <main/Elf.h>
#include <memory_management/paging.h>
#include <memory_management/memory.h>
#include "elf_loader.h"

// Returns the Entry Point (RIP) of the loaded program
elf_loader_result load_user_elf(uint8_t* elf_data, uint64_t* user_pml4_phys) {

    elf_loader_result result = {0};
    
    Elf64_Ehdr* header = (Elf64_Ehdr*)elf_data;

    // 1. Verify Magic Bytes
    if (header->e_ident[0] != 0x7F || header->e_ident[1] != 'E' ||
        header->e_ident[2] != 'L'  || header->e_ident[3] != 'F') {
        // Handle error (e.g., print to serial and return 0)
        return result;
    }

    // 2. Locate the Program Headers
    Elf64_Phdr* phdr = (Elf64_Phdr*)(elf_data + header->e_phoff);

    // 3. Iterate through headers
    for (int i = 0; i < header->e_phnum; i++) {
        
        if (phdr[i].p_type == PT_LOAD) {
            
            // Calculate how many 4KB pages this segment needs
            uint64_t pages_needed = (phdr[i].p_memsz + 0xFFF) / 4096;
            
            // The starting virtual address requested by the ELF (e.g., 0x400000)
            uint64_t vaddr = phdr[i].p_vaddr;

            uint64_t remaining_filesz = phdr[i].p_filesz;
            uint64_t current_file_offset = phdr[i].p_offset;

            // 4. Allocate and Map Memory for the Segment
            for (uint64_t p = 0; p < pages_needed; p++) {
                uint64_t phys_page = (uint64_t)pmm_request_page();
                
                // Map it into the USER's page table!
                paging_map_page(
                    P2V_DIRECT(user_pml4_phys), 
                    vaddr + (p * 4096), 
                    phys_page, 
                    PT_FLAG_PRESENT | PT_FLAG_READ_WRITE | PT_FLAG_USER_SUPER, 
                    KB_4
                );

                uint64_t page_dest_virt = P2V_DIRECT(phys_page);
                memset((void*)page_dest_virt, 0, 4096);

                if (remaining_filesz > 0){
                    uint64_t copy_size = remaining_filesz < 4096 ? remaining_filesz : 4096;
                    memcpy((void*)page_dest_virt, elf_data + current_file_offset, copy_size);

                    current_file_offset += copy_size;
                    remaining_filesz -= copy_size;
                }

            }
                
            result.pages_needed += pages_needed;
        }
    }

    result.entry_point = header->e_entry;

    // Return the entry point (e.g., your _start function address)
    return result;
}