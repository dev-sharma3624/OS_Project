#include <typedefs.h>
#include <file_system/fat32.h>
#include <memory_management/pmm.h>
#include <memory_management/memory.h>
#include <drivers/nvme.h>
#include <libs/k_string.h>
#include <libs/k_printf.h>
#include <drivers/font_renderer.h>

uint16_t bytes_per_sector;
uint32_t sectors_per_cluster;
uint32_t data_start_lba;
uint64_t fat_start_lba;
uint64_t fat_size_in_sectors;

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
    fat_start_lba = partition_start_lba + bpb->reserved_sectors;
    bytes_per_sector = bpb->bytes_per_sector;
    fat_size_in_sectors = bpb->sectors_per_fat_32;

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


void fat32_read_fat_sector(uint32_t sector_offset, uint64_t buffer_phys) {
    uint64_t target_lba = fat_start_lba + sector_offset;
    nvme_read_sector(target_lba, buffer_phys);
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

    uint32_t bytes_read = 0;
    uint32_t file_size = directory[file_offset].file_size;

    k_printf("  -> Content: \"");

    while(bytes_read < file_size){

        // 3. Convert Cluster -> LBA
        uint64_t offset_sectors = (cluster - 2) * sectors_per_cluster;
        uint64_t file_start_lba = data_start_lba + offset_sectors;

        for(int i = 0; i < sectors_per_cluster; i++){
            uint64_t read_lba = file_start_lba + i;
            nvme_read_sector(read_lba, buffer_phys);


            char* content = (char*)buffer_virt;
            int loop_length;
            if((file_size - bytes_read) <= nvme_get_sector_size()){
                loop_length = (file_size - bytes_read);
            }else{
                loop_length = nvme_get_sector_size();
            }
            for(int b=0; b < loop_length; b++) {
                k_printf("%c", content[b]);
            }

            bytes_read += nvme_get_sector_size();
            if(bytes_read >= file_size){
                break;
            }
        }

        // calculate the location
        uint32_t fat_bytes_offset = cluster * 4; //each entry is 4 bytes
        uint32_t fat_sector_offset = fat_bytes_offset / bytes_per_sector;
        uint32_t ent_offset = fat_bytes_offset % bytes_per_sector;

        uint64_t fat_buffer_phys = (uint64_t)pmm_request_page();
        uint64_t fat_buffer_virt = P2V(fat_buffer_phys);
        memset((void*)fat_buffer_virt, 0, 4096);

        fat32_read_fat_sector(fat_sector_offset, fat_buffer_phys);

        uint32_t* table_entry = (uint32_t*)(fat_buffer_virt + ent_offset);

        if(*table_entry == 0x0FFFFFFF){
            pmm_free_page(fat_buffer_phys);
            break;
        }

        if(*table_entry >= 0x00000002 && *table_entry <= 0x0FFFFFEF){
            cluster = *table_entry;
        }

        pmm_free_page(fat_buffer_phys);

    }
    

    k_printf("\"\n");

    pmm_free_page(buffer_phys);
}

void fat32_set_fat_entry(uint32_t cluster, uint32_t value) {

    // calculate the location
    uint32_t fat_offset = cluster * 4; //each entry is 4 bytes
    uint32_t fat_sector = fat_start_lba + (fat_offset / bytes_per_sector);
    uint32_t ent_offset = fat_offset % bytes_per_sector;

    // loading the sector
    uint64_t buffer_phys = (uint64_t)pmm_request_page();
    uint64_t buffer_virt = P2V(buffer_phys);
    nvme_read_sector(fat_sector, buffer_phys);

    // pointing to that specific entry in the fat
    uint32_t* table_entry = (uint32_t*)(buffer_virt + ent_offset);
    
    // We want to preserve the top 4 bits (Reserved) just in case, 
    // but usually setting it to 0x0FFFFFFF for EOF is fine.
    *table_entry = value; 

    // saving it back to disk
    nvme_write_sector(fat_sector, buffer_phys);

    pmm_free_page(buffer_phys);
}

int fat32_add_directory_entry(char* filename, uint32_t cluster, uint32_t size) {

    uint64_t buffer_phys = (uint64_t)pmm_request_page();
    uint64_t buffer_virt = P2V(buffer_phys);
    
    // reading the root directory
    // only 1 sector
    // in future might need to read multiple sectors
    nvme_read_sector(data_start_lba, buffer_phys);

    fat32_directory_entry_t* directory = (fat32_directory_entry_t*)buffer_virt;
    int found_slot = -1;

    // find an empty slot
    for (int i = 0; i < 16; i++) {
        if (directory[i].name[0] == 0x00 || directory[i].name[0] == 0xE5) { //0x00 is empty location and 0xE5 is deleted file
            found_slot = i;
            break;
        }
    }

    if (found_slot == -1) {
        k_printf("Error: Root Directory is full!\n");
        pmm_free_page(buffer_phys);
        return -1;
    }

    // copying file name to the directory address
    memcpy(directory[found_slot].name, filename, 8);
    
    // 0x20 is Archive/File
    directory[found_slot].attributes = 0x20;
    
    // setting cluster fields
    directory[found_slot].cluster_high = (uint16_t)((cluster >> 16) & 0xFFFF);
    directory[found_slot].cluster_low  = (uint16_t)(cluster & 0xFFFF);
    
    // file size
    directory[found_slot].file_size = size;

    // writing back to the drive
    nvme_write_sector(data_start_lba, buffer_phys);

    pmm_free_page(buffer_phys);
    return 0;
}

uint32_t fat32_find_free_cluster() {
    
    uint64_t buffer_phys = (uint64_t)pmm_request_page();
    uint64_t buffer_virt = P2V(buffer_phys);
    uint32_t* fat_table = (uint32_t*)buffer_virt;

    int entries_per_sector = bytes_per_sector / 4; //each entry is 4 bytes

    // loop to scan scan the fat table sector wise
    for (int sector_idx = 0; sector_idx < fat_size_in_sectors; sector_idx++) {
        
        // reading the sector
        fat32_read_fat_sector(sector_idx, buffer_phys);

        // loop to read each entry in every sector
        for (int i = 0; i < entries_per_sector; i++) {
            
            // this is not address/location of cluster but the index inside the fat that has that cluster information
            uint32_t actual_cluster = (sector_idx * entries_per_sector) + i;

            // skip Cluster 0 and 1 (Reserved)
            if (actual_cluster < 2) continue;

            // check if empty
            if ((fat_table[i] & 0x0FFFFFFF) == 0x00000000) {
                pmm_free_page(buffer_phys);
                return actual_cluster;
            }
        }
    }

    k_printf("Error: No free clusters found in first 10 FAT sectors.\n");
    pmm_free_page(buffer_phys);
    return 0;
}

void fat32_create_file(char* filename, char* content, int size) {

    uint64_t cluster_size = sectors_per_cluster * bytes_per_sector;
    uint64_t required_clusters = (size + cluster_size - 1) / cluster_size;

    // finding an empty cluster by looking inside the fat table
    uint32_t prev_free_cluster = fat32_find_free_cluster();
    if (prev_free_cluster == 0) return;

    k_printf("Allocated Cluster %d for file.\n", prev_free_cluster);

    // marking it as used inside fat
    fat32_set_fat_entry(prev_free_cluster, 0x0FFFFFFF);

    // calculating lba using that cluster value
    uint64_t lba = data_start_lba + (prev_free_cluster - 2) * sectors_per_cluster;

    for(int s = 0; s < cluster_size / nvme_get_sector_size(); s++) {
        nvme_write_sector(
            lba + s, 
            V2P(content + s * nvme_get_sector_size()) // Offset for sector
        );
    }

    // update the directory
    fat32_add_directory_entry(filename, prev_free_cluster, size);

    for(int i = 1; i < required_clusters; i++){

        // finding an empty cluster by looking inside the fat table
        uint32_t next_free_cluster = fat32_find_free_cluster();
        if (next_free_cluster == 0) return;

        k_printf("Allocated Cluster %d for file.\n", next_free_cluster);

        // marking it as used inside fat
        fat32_set_fat_entry(prev_free_cluster, next_free_cluster);
        fat32_set_fat_entry(next_free_cluster, 0x0FFFFFFF);

        // calculating lba using that cluster value
        uint64_t lba = data_start_lba + (next_free_cluster - 2) * sectors_per_cluster;

        char* current_buffer_pos = content + (i * cluster_size);

        for(int s = 0; s < cluster_size / nvme_get_sector_size(); s++) {
            nvme_write_sector(
                lba + s, 
                V2P(current_buffer_pos + s * nvme_get_sector_size()) // Offset for sector
            );
        }

        prev_free_cluster = next_free_cluster;

    }
    
    k_printf("File %s written successfully.\n", filename);
}

void fat32_test_write() {
    k_printf("\n--- STARTING FAT32 WRITE TEST ---\n");

    char* filename = "TEST    TXT"; 
    char* content = "This is a message from your OS kernel!\nIt proves that write support works.";
    int size = k_strlen(content);

    k_printf("Attempting to write file '%s' (%d bytes)...\n", filename, size);
    
    uint64_t buf_phys = (uint64_t)pmm_request_page();
    uint64_t buf_virt = P2V(buf_phys);
    memcpy((void*)buf_virt, content, size);

    fat32_create_file(filename, (char*)buf_virt, size);

    pmm_free_page(buf_phys);

    k_printf("Write done. Attempting to read back...\n");
    
    fat32_directory_entry_t* root_dir;
    fat32_get_directory(&root_dir, data_start_lba);
    
    fat32_read_file(root_dir, filename);

    k_printf("--- TEST COMPLETE ---\n");
}

void test_impl(){

    fat32_directory_entry_t* root_directory_entry;
    fat32_get_directory(&root_directory_entry, data_start_lba);

    fat32_list_all_files(root_directory_entry);

    char* file_name = "TEST    ";

    fat32_read_file(root_directory_entry, file_name);

    pmm_free_page(V2P((uint64_t)root_directory_entry));

}


void fat32_init(uint64_t partition_start_lba) {
    fat32_read_bpb(partition_start_lba);
    font_renderer_clear_screen();
    test_impl();
    // fat32_test_write();
}