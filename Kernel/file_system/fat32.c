#include <typedefs.h>
#include <file_system/fat32.h>
#include "fat32_calc.h"
#include "ft_nvm_bridge.h"
#include <memory_management/pmm.h>
#include <memory_management/memory.h>
#include <memory_management/heap.h>
#include <drivers/nvme_interface.h>
#include <libs/k_string.h>
#include <libs/k_printf.h>
#include <drivers/font_renderer.h>

#define DIRECTORY_SELF_POINTER ".       "
#define DIRECTORY_PARENT_POINTER "..      "

void fat32_read_bpb(uint64_t partition_start_lba){

    k_printf("\nFAT32: Reading BPB from LBA %d...\n", partition_start_lba);

    uint64_t buffer_virt = (uint64_t) heap_kmalloc(512); //bpb is always 512 bytes

    ft_nvm_bridge_force_read(buffer_virt, partition_start_lba, 512);

    fat32_bpb_t* bpb = (fat32_bpb_t*)buffer_virt;

    // Validate the Signature (Byte 510 = 0x55, Byte 511 = 0xAA)
    uint8_t* raw = (uint8_t*)buffer_virt;
    if (raw[510] != 0x55 || raw[511] != 0xAA) {
        k_printf("FAT32: Invalid Boot Signature! Found %x %x (Expected 0x55 0xAA)\n", raw[510], raw[511]);
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

    fat32_calc_init_parameters(bpb, partition_start_lba);

    heap_kfree((void*)buffer_virt);

}

/* 
******************************************************************************************************
******************************************************************************************************
******************************************************************************************************
******************************************************************************************************
******************************************************************************************************
******************************************************************************************************

These functions are for operations performed on the file allocation table present between the reserved
sectors and data region.

******************************************************************************************************
******************************************************************************************************
******************************************************************************************************
******************************************************************************************************
******************************************************************************************************
******************************************************************************************************
 */

//retursn the cluster number linked with the current cluster inside FAT
uint32_t fat32_find_next_cluster(uint32_t prev_cluster){

    uint16_t size = fat32_calc_get_bytes_per_sector();
    uint64_t buffer = (uint64_t) heap_kmalloc(size);

    uint64_t sector_entry_lba_offsets[3] = {0, 0, 0};
    fat32_calc_fat_table_offsets(sector_entry_lba_offsets, prev_cluster);

    ft_nvm_bridge_read(buffer, sector_entry_lba_offsets[2], size);

    uint32_t* table_entry = (uint32_t*)(buffer + sector_entry_lba_offsets[1]);

    uint32_t next_cluster = *table_entry & 0x0FFFFFFF;
    heap_kfree((void*) buffer);
    return next_cluster;
}

void fat32_set_fat_entry(uint32_t cluster, uint32_t value) {

    uint16_t size = fat32_calc_get_bytes_per_sector();
    uint64_t buffer = (uint64_t) heap_kmalloc(size);

    uint64_t sector_entry_lba_offsets[3] = {0, 0, 0};
    fat32_calc_fat_table_offsets(sector_entry_lba_offsets, cluster);

    ft_nvm_bridge_read(buffer, sector_entry_lba_offsets[2], size);

    uint32_t* table_entry = (uint32_t*)(buffer + sector_entry_lba_offsets[1]);
    *table_entry = value; 

    ft_nvm_bridge_write(buffer, sector_entry_lba_offsets[2], size);

    heap_kfree(buffer);
}

uint32_t fat32_find_free_cluster() {

    uint16_t size = fat32_calc_get_bytes_per_sector();
    uint64_t buffer = (uint64_t) heap_kmalloc(size);
    uint32_t* fat_table = (uint32_t*)buffer;

    int entries_per_sector = size / 4; //each entry is 4 bytes

    // loop to scan scan the fat table sector wise
    for (int sector_idx = 0; sector_idx < fat32_calc_get_fat_size_in_sectors(); sector_idx++) {

        ft_nvm_bridge_read(buffer, fat32_calc_get_fat_start_lba() + sector_idx, size);

        // loop to read each entry in every sector
        for (int i = 0; i < entries_per_sector; i++) {
            
            // this is not address/location of cluster but the index inside the fat that has that cluster information
            uint32_t actual_cluster = (sector_idx * entries_per_sector) + i;

            // skip Cluster 0 and 1 (Reserved)
            if (actual_cluster < 2) continue;

            // check if empty
            if ((fat_table[i] & 0x0FFFFFFF) == 0x00000000) {
                fat32_zero_cluster(actual_cluster);
                heap_kfree(buffer);
                return actual_cluster;
            }
        }
    }

    heap_kfree(buffer);
    return 0;
}

/* 
******************************************************************************************************
******************************************************************************************************
******************************************************************************************************
******************************************************************************************************
******************************************************************************************************
******************************************************************************************************

These functions are helpers.

******************************************************************************************************
******************************************************************************************************
******************************************************************************************************
******************************************************************************************************
******************************************************************************************************
******************************************************************************************************
 */

//scope of search: one sector, i.e, entries that can physically fit inside one sector
//returns offset withing the current sector if found, returns -1 if not found
int fat32_match_file(fat32_directory_entry_t* directory, char* file_name, int n){

    // number of entries in each sector of file system
    uint16_t n_entry = fat32_calc_get_bytes_per_sector() / sizeof(fat32_directory_entry_t);

    for (int i = 0; i < n_entry; i++) {

        char* name = directory[i].name;

        if (k_strncmp(name, file_name, n) == 0) {

            return i;
            
        }

    }

    return -1;
}

//returns a 64-bit aggregate of cluster(lower 32 bits) and file_size(upper 32 bits), returns 0 if file not found
uint64_t fat32_find_file(uint32_t dir_loc_cluster, char* file_name){

    uint64_t dir_sector_lba_offset[2] = {0, 0};
    fat32_calc_dir_offsets(dir_sector_lba_offset, dir_loc_cluster);

    uint32_t current_cluster = dir_loc_cluster;
    uint64_t size = fat32_calc_get_bytes_per_sector();
    uint64_t buffer = heap_kmalloc(size);

    do {

        //reading data sector wise
        for (uint32_t i = 0; i < fat32_calc_get_fat_sectors_per_cluster(); i++){

            //adjusting lba according to the sector number
            uint64_t lba = dir_sector_lba_offset[1] + i;
            // nvme_read_sector(lba, buffer_phys);
            ft_nvm_bridge_read(buffer, lba, size);

            fat32_directory_entry_t* directory = (fat32_directory_entry_t*) buffer;

            //returns offset at which file name match was found, if not found returns -1
            int file_index = fat32_match_file(directory, file_name, 8);

            // if not -1, then valid match found
            if (file_index != -1){
                heap_kfree(buffer);
                uint32_t file_cluster = ((uint32_t)directory[file_index].cluster_high << 16) | (uint32_t)directory[file_index].cluster_low;
                uint32_t file_size = directory[file_index].file_size;
                return (uint64_t) ((uint64_t)file_size << 32) | (uint64_t) file_cluster;
            }
            

        }

        //after reading sectors of one cluster, moving to next cluster in case file is saved in multiple clusters
        //returns valid cluster number if there is cluster ahead and returns EOF if it is the end cluster of current directory
        current_cluster = fat32_find_next_cluster(current_cluster);
        if(current_cluster >= EOF){
            break;
        }
        fat32_calc_dir_offsets(dir_sector_lba_offset, current_cluster);

    } while (true);

    heap_kfree(buffer);
    return 0;

}

/* 
******************************************************************************************************
******************************************************************************************************
******************************************************************************************************
******************************************************************************************************
******************************************************************************************************
******************************************************************************************************

These functions are for read/write operations in the data region.

******************************************************************************************************
******************************************************************************************************
******************************************************************************************************
******************************************************************************************************
******************************************************************************************************
******************************************************************************************************
 */

void fat32_read_file(uint32_t cluster, char* file_name, uint64_t buffer){
    
    uint64_t file_cluster_size = fat32_find_file(cluster, file_name);
    uint32_t file_cluster = (uint32_t) (file_cluster_size & 0xFFFFFFFF);
    uint32_t file_size = (uint32_t) (file_cluster_size >> 32);
    uint32_t bytes_read = 0;

    if(file_cluster == 0){
        k_printf("File not found!\n");
        return;
    }

    uint64_t file_sector_lba_offsets[2] = {0, 0};
    fat32_calc_dir_offsets(file_sector_lba_offsets, file_cluster);
    uint64_t file_sector_offset = file_sector_lba_offsets[0];
    uint64_t file_start_lba = file_sector_lba_offsets[1];

    uint64_t current_memory_dest = buffer;

    while(bytes_read < file_size){

        for(int i = 0; i < fat32_calc_get_fat_sectors_per_cluster(); i++){

            uint64_t read_lba = file_start_lba + i;
            ft_nvm_bridge_read(current_memory_dest, read_lba, fat32_calc_get_bytes_per_sector());

            if((file_size - bytes_read) <= fat32_calc_get_bytes_per_sector()){
                bytes_read += (file_size - bytes_read);
            }else{
                bytes_read += fat32_calc_get_bytes_per_sector();
            }

            current_memory_dest += fat32_calc_get_bytes_per_sector();

            if(bytes_read >= file_size){
                break;
            }
        }

        file_cluster = fat32_find_next_cluster(file_cluster);
        if(file_cluster == EOF){
            break;
        }
        fat32_calc_dir_offsets(file_sector_lba_offsets, file_cluster);
        file_sector_offset = file_sector_lba_offsets[0];
        file_start_lba = file_sector_lba_offsets[1];

    }
    

    k_printf("\n\n");

    if(bytes_read != file_size){
        k_printf("ERROR: File read bytes and file size mismatch\n");
    }
}

void fat32_set_dir_ent_values(fat32_directory_entry_t* directory, int found_slot, char* file_name, uint8_t attr, uint32_t cluster, uint32_t file_size ){
    
    // copying file name to the directory address
    memcpy(directory[found_slot].name, file_name, 8);
    memcpy(directory[found_slot].ext, file_name + 8, 3);
    
    // 0x20 is Archive/File
    directory[found_slot].attributes = attr;
    
    // setting cluster fields
    directory[found_slot].cluster_high = (uint16_t)((cluster >> 16) & 0xFFFF);
    directory[found_slot].cluster_low  = (uint16_t)(cluster & 0xFFFF);
    
    // file size
    directory[found_slot].file_size = file_size;
}


int fat32_add_directory_entry(char* filename, uint32_t directory_loc_cluster, uint32_t file_loc_cluster, uint32_t size, uint8_t attr) {

    uint64_t buffer = heap_kmalloc(fat32_calc_get_bytes_per_sector());

    uint32_t dir_cluster = directory_loc_cluster;
    uint64_t dir_sector_lba_offsets[2] = {0, 0};
    fat32_calc_dir_offsets(dir_sector_lba_offsets, directory_loc_cluster);
    uint32_t dir_sector_offset = dir_sector_lba_offsets[0];
    uint64_t dir_start_lba = dir_sector_lba_offsets[1];

    char empty_marker = 0x00;
    char deleted_marker = 0xE5;

    do {

        //reading data sector wise
        for (uint32_t i = 0; i < fat32_calc_get_fat_sectors_per_cluster(); i++){

            //adjusting lba according to the sector number
            uint64_t lba = dir_start_lba + i;
            ft_nvm_bridge_read(buffer, lba, fat32_calc_get_bytes_per_sector());

            fat32_directory_entry_t* directory = (fat32_directory_entry_t*) buffer;

            //returns offset at which file name match was found, if not found returns -1
            int file_index = fat32_match_file(directory, &empty_marker, 1);

            // if not -1, then valid match found
            if (file_index != -1){

                fat32_set_dir_ent_values(
                    directory,
                    file_index,
                    filename,
                    attr,
                    file_loc_cluster,
                    size
                );
  
                // writing back to the drive
                ft_nvm_bridge_write(buffer, lba, fat32_calc_get_bytes_per_sector());
                heap_kfree(buffer);

                return 1;
            }

            file_index = fat32_match_file(directory, &deleted_marker, 1);

            // if not -1, then valid match found
            if (file_index != -1){

                fat32_set_dir_ent_values(
                    directory,
                    file_index,
                    filename,
                    attr,
                    file_loc_cluster,
                    size
                );
                
                // writing back to the drive
                ft_nvm_bridge_write(buffer, lba, fat32_calc_get_bytes_per_sector());
                heap_kfree(buffer);
                return 1;
            }
            

        }

        //after reading sectors of one cluster, moving to next cluster in case file is saved in multiple clusters
        //returns valid cluster number if there is cluster ahead and returns EOF if it is the end cluster of current directory
        dir_cluster= fat32_find_next_cluster(dir_cluster);
        if(dir_cluster >= EOF){
            break;
        }
        //calcualting the lba for the next cluster
        fat32_calc_dir_offsets(dir_sector_lba_offsets, dir_cluster);
        dir_sector_offset = dir_sector_lba_offsets[0];
        dir_start_lba = dir_sector_lba_offsets[1];

    } while (true);

    heap_kfree(buffer);
    // pmm_free_page(buffer_phys);
    return -1;
}

void fat32_zero_cluster(uint32_t cluster_number) {
    uint64_t buffer = heap_kmalloc(fat32_calc_get_bytes_per_sector());

    uint64_t dir_sector_lba_offsets[2] = {0, 0};
    fat32_calc_dir_offsets(dir_sector_lba_offsets, cluster_number);
    uint32_t sector_offset = dir_sector_lba_offsets[0];
    uint64_t lba = dir_sector_lba_offsets[1];

    for (uint32_t i = 0; i < fat32_calc_get_fat_sectors_per_cluster(); i++) {
        ft_nvm_bridge_write(buffer, lba+i, fat32_calc_get_bytes_per_sector());
    }

    heap_kfree(buffer);
}

int fat32_create_file(char* filename, char* content, int size, uint32_t dir_loc_cluster, uint8_t attr) {

    uint64_t existing_file = fat32_find_file(dir_loc_cluster, filename);

    if (existing_file != 0) {
        return -1; 
    }

    uint64_t cluster_size = fat32_calc_get_fat_sectors_per_cluster() * fat32_calc_get_bytes_per_sector();
    uint64_t required_clusters = (size + cluster_size - 1) / cluster_size;

    // finding an empty cluster by looking inside the fat table
    uint32_t initial_cluster = fat32_find_free_cluster();
    if (initial_cluster == 0) return -1;

    // marking it as used inside fat
    fat32_set_fat_entry(initial_cluster, EOF);

    uint64_t dir_sector_lba_offsets[2] = {0, 0};
    fat32_calc_dir_offsets(dir_sector_lba_offsets, initial_cluster);
    uint32_t sector_offset = dir_sector_lba_offsets[0];
    uint64_t lba = dir_sector_lba_offsets[1];

    uint32_t required_sectors = (size / fat32_calc_get_bytes_per_sector()) + 1;
    required_sectors = (required_sectors < fat32_calc_get_fat_sectors_per_cluster()) ? required_sectors : fat32_calc_get_fat_sectors_per_cluster();
    for(int s = 0; s < required_sectors; s++) {
        ft_nvm_bridge_write(content + (s * fat32_calc_get_bytes_per_sector()), lba + s, fat32_calc_get_bytes_per_sector());
    }

    uint32_t previous_cluster = initial_cluster;

    for(int i = 1; i < required_clusters; i++){

        // finding an empty cluster by looking inside the fat table
        uint32_t next_free_cluster = fat32_find_free_cluster();
        if (next_free_cluster == 0) return -1;


        // marking it as used inside fat
        fat32_set_fat_entry(previous_cluster, next_free_cluster);
        fat32_set_fat_entry(next_free_cluster, EOF);

        fat32_calc_dir_offsets(dir_sector_lba_offsets, initial_cluster);
        sector_offset = dir_sector_lba_offsets[0];
        lba = dir_sector_lba_offsets[1];

        char* current_buffer_pos = content + (i * cluster_size);

        for(int s = 0; s < fat32_calc_get_fat_sectors_per_cluster(); s++) {
            ft_nvm_bridge_write(content + (s * fat32_calc_get_bytes_per_sector()), lba + s, fat32_calc_get_bytes_per_sector());
        }

        previous_cluster = next_free_cluster;

    }

    // update the directory
    fat32_add_directory_entry(filename, dir_loc_cluster, initial_cluster, size, attr);
    
    return 0;
}

void fat32_create_dir(char* dirName, uint32_t parent_dir_cluster){

    uint64_t existing_file = fat32_find_file(parent_dir_cluster, dirName);

    if (existing_file != 0) {
        k_printf("Error: Name already exists.\n");
        return; 
    }

    // finding an empty cluster by looking inside the fat table
    uint32_t initial_cluster = fat32_find_free_cluster();
    if (initial_cluster == 0) return;

    // marking it as used inside fat
    fat32_set_fat_entry(initial_cluster, 0x0FFFFFFF);

    uint64_t dir_sector_lba_offsets[2] = {0, 0};
    fat32_calc_dir_offsets(dir_sector_lba_offsets, initial_cluster);
    uint32_t sector_offset = dir_sector_lba_offsets[0];
    uint64_t lba = dir_sector_lba_offsets[1];

    fat32_add_directory_entry(dirName, parent_dir_cluster, initial_cluster, 0, DIR_ATTR);
    fat32_add_directory_entry(DIRECTORY_SELF_POINTER, initial_cluster, initial_cluster, 0, DIR_ATTR);
    fat32_add_directory_entry(DIRECTORY_PARENT_POINTER, initial_cluster, parent_dir_cluster, 0, DIR_ATTR);

}

void fat32_list_all_entries(uint32_t dir_cluster){

    uint64_t buffer = heap_kmalloc(fat32_calc_get_bytes_per_sector());
    uint64_t dir_sector_lba_offsets[2] = {0, 0};
    fat32_calc_dir_offsets(dir_sector_lba_offsets, dir_cluster);
    uint32_t dir_sector_offset = dir_sector_lba_offsets[0];
    uint64_t dir_start_lba = dir_sector_lba_offsets[1];

    uint64_t entries_per_sector = fat32_calc_get_bytes_per_sector() / sizeof(fat32_directory_entry_t);

    while(true){

        for(int i = 0; i < fat32_calc_get_fat_sectors_per_cluster(); i++){

            uint64_t read_lba = dir_start_lba + i;
            ft_nvm_bridge_read(buffer, read_lba, fat32_calc_get_bytes_per_sector());
            // nvme_read_sector(read_lba, buffer_phys);


            fat32_directory_entry_t* directory = (fat32_directory_entry_t*)buffer;

            for (int i = 0; i < entries_per_sector; i++) {
                if (directory[i].name[0] == 0x00) break;

                // Check for Deleted File (0xE5)
                if (directory[i].name[0] == 0xE5) continue;

                // Check for Long File Name entry (Attribute 0x0F)
                // (We skip these for now to keep it simple)
                if (directory[i].attributes == 0x0F) continue;

                // Print the Name
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

        uint32_t next_cluster = fat32_find_next_cluster(dir_cluster);
        if(next_cluster >= EOF){
            break;
        }
        fat32_calc_dir_offsets(dir_sector_lba_offsets, dir_cluster);
        dir_sector_offset = dir_sector_lba_offsets[0];
        dir_start_lba = dir_sector_lba_offsets[1];

    }
    
    k_printf("\n\n");

    heap_kfree(buffer);
}

/* void fat32_test_write() {
    k_printf("\n--- STARTING FAT32 WRITE TEST ---\n");

    char* filename = "TEST    TXT"; 
    char* content = "This is a message from your OS kernel!\nIt proves that write support works.";
    int size = k_strlen(content);

    k_printf("Attempting to write file '%s' (%d bytes)...\n", filename, size);
    
    uint64_t buf_phys = (uint64_t)pmm_request_page();
    uint64_t buf_virt = P2V(buf_phys);
    memset(buf_virt, 0, 4096);
    memcpy((void*)buf_virt, content, size);
    k_printf("buffer starts at: %p\n", buf_phys);
    k_printf("bp1\n");
    fat32_create_file(filename, (char*)buf_virt, size, 2, FILE_ATTR);
    k_printf("bp2\n");
    fat32_create_dir("dir1       ", 2);
    k_printf("bp3\n");
    uint32_t cluster = (uint32_t) (fat32_find_file(2, "dir1       ") & 0xFFFFFFFF);
    k_printf("bp4\n");
    fat32_create_file("TEST1   TXT", (char*)buf_virt, size, cluster, FILE_ATTR);

    pmm_free_page(buf_phys);

    k_printf("Write done. Attempting to read back...\n");
    
    fat32_read_file(2, filename);

    k_printf("--- TEST COMPLETE ---\n");
}

void test_impl(){

    char* file_name = "TEST    ";

    fat32_read_file(2, file_name);

} */


void fat32_init(uint64_t partition_start_lba) {
    fat32_read_bpb(partition_start_lba);
    // font_renderer_clear_screen();
    // test_impl();
    // fat32_test_write();
    // font_renderer_clear_screen();
}