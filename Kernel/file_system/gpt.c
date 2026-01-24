#include <typedefs.h>
#include <file_system/gpt.h>
#include <memory_management/pmm.h>
#include <memory_management/memory.h>
#include <drivers/nvme.h>
#include <libs/k_printf.h>

void gpt_scan_partition_table(uint32_t nsid){
    k_printf("\n\nGPT: Starting Scan on NSID %d...\n", nsid);

    // requesting memory
    uint64_t buffer_phy_addr = (uint64_t) pmm_request_page();
    uint64_t buffer_virt_addr = P2V(buffer_phy_addr);
    memset(buffer_virt_addr, 0, 4096);

    //GPT header is always at LBA 1
    nvme_read_sector(1, buffer_phy_addr);

    gpt_header_t* gpt_header = (gpt_header_t*) buffer_virt_addr;

    //verifying the signature that whether lba 1 has efi part or not
    uint64_t gpt_signature = *(uint64_t*) gpt_header->signature;
    if(gpt_signature != GPT_SIGNATURE){
        k_printf("GPT: Signature Mismatch! Not a valid GPT disk.\n");
        pmm_free_page(buffer_phy_addr);
        return;
    }

    uint32_t num_entries = gpt_header->num_partition_entries;
    uint32_t entry_size = gpt_header->size_partition_entry; 
    uint64_t table_lba = gpt_header->partition_entry_lba;

    k_printf("GPT: Valid. Found %d entries starting at LBA %d.\n", num_entries, table_lba);

    k_printf("DEBUG: Entry Size: %d, Num Entries: %d, Table LBA: %d\n", 
             gpt_header->size_partition_entry, 
             gpt_header->num_partition_entries,
             gpt_header->partition_entry_lba);

    //calculating number of entries per sector
    int entries_per_sector = nvme_get_sector_size() / entry_size; 
    int current_lba = table_lba;
    int entries_checked = 0;

    memset(buffer_virt_addr, 0, 4096);

    k_printf("GPT: Parsing Partition Table...\n");

    while(entries_checked < num_entries){

        nvme_read_sector(current_lba, buffer_phy_addr);

        uint8_t* ptr = (uint8_t*) buffer_virt_addr;

        for(int i = 0; i < entries_per_sector; i++){

            if(entries_checked >= num_entries) break;

            gpt_entry_t* entry = (gpt_entry_t*) ptr;

            if (((uint64_t*)entry->type_guid)[0] != 0) { //checking if the entry is active or not, 0 means this entry is not being used
                
                k_printf("  [P%d] Start: %d | End: %d | Name: ", 
                         entries_checked, entry->starting_lba, entry->ending_lba);
                
                // Print Name (ASCII only)
                for(int c=0; c<36; c++) {
                   // UTF-16: The char is in the first byte, second byte is 0 (for English)
                   char ascii = (char)entry->partition_name[c]; 
                   if(ascii == 0) break;
                   k_printf("%c", ascii);
                }
                k_printf("\n");
            }

            ptr += entry_size; //moving ptr by exactly entry_size bytes ahead
            entries_checked++;

        }

        current_lba++;

    }

    pmm_free_page(buffer_phy_addr);
    k_printf("GPT: Scan Complete.\n");
}