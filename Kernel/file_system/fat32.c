#include <typedefs.h>
#include <file_system/fat32.h>
#include <memory_management/pmm.h>
#include <memory_management/memory.h>
#include <drivers/nvme.h>
#include <libs/k_string.h>
#include <libs/k_printf.h>

uint32_t sectors_per_cluster;
uint32_t data_start_lba;

void fat32_read_bpb(uint64_t partition_start_lba){

    k_printf("\nFAT32: Reading BPB from LBA %d...\n", partition_start_lba);

    //getting physical memory to read data into
    uint64_t buffer_phys = (uint64_t)pmm_request_page();
    uint64_t buffer_virt = P2V(buffer_phys);
    memset((void*)buffer_virt, 0, 4096);


    // The BPB is strictly at offset 0 of the partition.
    nvme_read_sector(partition_start_lba, buffer_phys);

    fat32_bpb_t* bpb = (fat32_bpb_t*)buffer_virt;

    // Validate the Signature (Byte 510 = 0x55, Byte 511 = 0xAA)
    uint8_t* raw = (uint8_t*)buffer_virt;
    if (raw[510] != 0x55 || raw[511] != 0xAA) {
        k_printf("FAT32: Invalid Boot Signature! Found %x %x (Expected 0x55 0xAA)\n", raw[510], raw[511]);
        pmm_free_page(buffer_phys);
        return;
    }

    k_printf("FAT32: Valid Filesystem Found.\n");
    
    // OEM Name
    k_printf("  OEM Name: ");
    for(int i=0; i<8; i++) k_printf("%c", bpb->oem_name[i]);
    k_printf("\n");

    k_printf("  Bytes Per Sector:    %d\n", bpb->bytes_per_sector);
    k_printf("  Sectors Per Cluster: %d\n", bpb->sectors_per_cluster);
    k_printf("  Reserved Sectors:    %d\n", bpb->reserved_sectors);
    k_printf("  Number of FATs:      %d\n", bpb->fat_count);
    k_printf("  Sectors Per FAT:     %d\n", bpb->sectors_per_fat_32);
    k_printf("  Root Dir Cluster:    %d\n", bpb->root_cluster);

    // Check if it's actually FAT32 (Total Sectors 16 must be 0)
    if (bpb->total_sectors_16 != 0) {
        k_printf("WARNING: This looks like FAT16, not FAT32!\n");
    }

    //required for calculating offset
    sectors_per_cluster = bpb->sectors_per_cluster;

    // Calculate Data Area Start
    uint32_t fat_size = bpb->fat_count * bpb->sectors_per_fat_32;
    data_start_lba = partition_start_lba + bpb->reserved_sectors + fat_size;

    k_printf("FAT32: Data Area starts at LBA %d\n", data_start_lba);

    pmm_free_page(buffer_phys);

}

void fat32_get_directory(fat32_directory_entry_t** directory_entry, uint64_t data_start_lba){

    uint64_t buffer_phys = (uint64_t)pmm_request_page();
    uint64_t buffer_virt = P2V(buffer_phys);
    memset((void*)buffer_virt, 0, 4096);
    
    // Read the first sector of the Root Directory
    nvme_read_sector(data_start_lba, buffer_phys);

    *directory_entry = (fat32_directory_entry_t*) buffer_virt;

}

void fat32_list_all_files(fat32_directory_entry_t* directory){

    k_printf("FAT32: Listing Root Directory:\n");

    // A sector is 512 bytes, each entry is 32 bytes -> 16 entries per sector
    for (int i = 0; i < 16; i++) {
        if (directory[i].name[0] == 0x00) break;

        // 2. Check for Deleted File (0xE5)
        if (directory[i].name[0] == 0xE5) continue;

        // 3. Check for Long File Name entry (Attribute 0x0F)
        // (We skip these for now to keep it simple)
        if (directory[i].attributes == 0x0F) continue;

        // 4. Print the Name
        k_printf("  FILE: \"");
        for (int j = 0; j < 8; j++) {
            if (directory[i].name[j] != ' ') k_printf("%c", directory[i].name[j]);
        }
        k_printf(".");
        for (int j = 0; j < 3; j++) {
            if (directory[i].ext[j] != ' ') k_printf("%c", directory[i].ext[j]);
        }
        k_printf("\"  (Size: %d bytes)\n", directory[i].file_size);

    }

}

int fat32_find_file(fat32_directory_entry_t* directory, char* file_name){

    // A sector is 512 bytes, each entry is 32 bytes -> 16 entries per sector
    for (int i = 0; i < 16; i++) {

        char* name = directory[i].name;

        if (k_strncmp(name, file_name, 8) == 0) {
            return i;
        }

    }

    return -1;

}

void fat32_read_file(fat32_directory_entry_t* directory, char* file_name){

    uint64_t buffer_phys = (uint64_t)pmm_request_page();
    uint64_t buffer_virt = P2V(buffer_phys);
    memset((void*)buffer_virt, 0, 4096);
    
    int file_offset = fat32_find_file(directory, file_name);

    if(file_offset == -1){
        k_printf("File not found!\n");
        return;
    }

    k_printf("\nFAT32: Found File! Reading content...\n");

    // 2. Extract the Start Cluster
    // It's split into two 16-bit fields. Combine them.
    uint32_t cluster = ((uint32_t)directory[file_offset].cluster_high << 16) | 
                    (uint32_t)directory[file_offset].cluster_low;

    // 3. Convert Cluster -> LBA
    uint64_t offset_sectors = (cluster - 2) * sectors_per_cluster;
    uint64_t file_lba = data_start_lba + offset_sectors;
    uint32_t file_size = directory[file_offset].file_size;
    k_printf("  -> File starts at Cluster %d (LBA %d)\n", cluster, file_lba);

    // 4. Read the content
    // We reuse buffer_phys/virt for simplicity, overwriting the directory list
    nvme_read_sector(file_lba, buffer_phys);

    // 5. Print it!
    k_printf("  -> Content: \"");
    char* content = (char*)buffer_virt;
    
    // Print only the file size amount of characters
    for(int b=0; b < file_size; b++) {
        k_printf("%c", content[b]);
    }
    k_printf("\"\n");

    pmm_free_page(buffer_phys);
}

void test_impl(){

    fat32_directory_entry_t* root_directory_entry;
    fat32_get_directory(&root_directory_entry, data_start_lba);

    fat32_list_all_files(root_directory_entry);

    char* file_name = "DATA    ";

    fat32_read_file(root_directory_entry, file_name);

    pmm_free_page(V2P((uint64_t)root_directory_entry));

}


void fat32_init(uint64_t partition_start_lba) {
    fat32_read_bpb(partition_start_lba);
    test_impl();
}