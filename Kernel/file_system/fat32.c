#include <typedefs.h>
#include <file_system/fat32.h>
#include <memory_management/pmm.h>
#include <memory_management/memory.h>
#include <memory_management/heap.h>
#include <drivers/nvme.h>
#include <libs/k_string.h>
#include <libs/k_printf.h>
#include <drivers/font_renderer.h>

uint16_t bytes_per_sector;
uint32_t sectors_per_cluster;
uint64_t data_start_lba;
uint64_t fat_start_lba;
uint64_t fat_size_in_sectors;

#define EOF 0x0FFFFFF8
#define FILE_ATTR 0x20
#define DIR_ATTR 0x10

#define FAT_BYTES_OFFSET(cluster_no) (cluster_no * 4)
#define FAT_SECTOR_OFFSET(fat_bytes_offset) (fat_bytes_offset / bytes_per_sector)
#define FAT_ENTRY_OFFSET(fat_bytes_offset) (fat_bytes_offset % bytes_per_sector)

#define FILE_SECTOR_OFFSET(cluster_no) ((cluster_no - 2) * sectors_per_cluster)
#define FILE_START_LBA(sector_offset) (sector_offset + data_start_lba)


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

//reads a NVMe sized sector and into the provided buffer
void fat32_read_fat_sector(uint32_t sector_offset, uint64_t buffer_phys) {
    uint64_t target_lba = fat_start_lba + sector_offset;
    nvme_read_sector(target_lba, buffer_phys);
}

//retursn the cluster number linked with the current cluster inside FAT
uint32_t fat32_find_next_cluster(uint32_t prev_cluster){

    uint64_t buffer_phys = (uint64_t)pmm_request_page();
    uint64_t buffer_virt = P2V(buffer_phys);
    memset((void*)buffer_virt, 0, 4096);

    uint32_t fat_byte_offset = FAT_BYTES_OFFSET(prev_cluster);
    uint32_t fat_sector_offset = FAT_SECTOR_OFFSET(fat_byte_offset);
    uint32_t ent_offset = FAT_ENTRY_OFFSET(fat_byte_offset);

    fat32_read_fat_sector(fat_sector_offset, buffer_phys);

    uint32_t* table_entry = (uint32_t*)(buffer_virt + ent_offset);

    uint32_t next_cluster = *table_entry & 0x0FFFFFFF;
    pmm_free_page(buffer_phys);
    return next_cluster;

}

void fat32_set_fat_entry(uint32_t cluster, uint32_t value) {

    // calculate the location
    uint32_t fat_byte_offset = FAT_BYTES_OFFSET(cluster); //each entry is 4 bytes
    uint32_t fat_sector_offset = FAT_SECTOR_OFFSET(fat_byte_offset);
    uint32_t ent_offset = FAT_ENTRY_OFFSET(fat_byte_offset);

    // loading the sector
    uint64_t buffer_phys = (uint64_t)pmm_request_page();
    uint64_t buffer_virt = P2V(buffer_phys);
    k_printf("set fat entry read\n");
    fat32_read_fat_sector(fat_sector_offset, buffer_phys);

    // pointing to that specific entry in the fat
    uint32_t* table_entry = (uint32_t*)(buffer_virt + ent_offset);
    
    // We want to preserve the top 4 bits (Reserved) just in case, 
    // but usually setting it to 0x0FFFFFFF for EOF is fine.
    *table_entry = value; 

    k_printf("set fat entry write\n");
    // saving it back to disk
    nvme_write_sector(fat_start_lba + fat_sector_offset, buffer_phys);
    k_printf("set fat entry write end\n");

    pmm_free_page(buffer_phys);
}

uint32_t fat32_find_free_cluster() {
    
    uint64_t buffer_phys = (uint64_t)pmm_request_page();
    uint64_t buffer_virt = P2V(buffer_phys);
    memset((void*) buffer_virt, 0, 4096);
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
                fat32_zero_cluster(actual_cluster);
                pmm_free_page(buffer_phys);
                return actual_cluster;
            }
        }
    }

    k_printf("Error: No free clusters found in first 10 FAT sectors.\n");
    pmm_free_page(buffer_phys);
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
    uint16_t n_entry = bytes_per_sector / sizeof(fat32_directory_entry_t);

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
    k_printf("find file\n");

    //the first lba at which file chain begins in data region
    uint32_t dir_sector_offset = FILE_SECTOR_OFFSET(dir_loc_cluster);
    uint64_t dir_start_lba = FILE_START_LBA(dir_sector_offset);

    uint64_t buffer_phys = (uint64_t)pmm_request_page();
    uint64_t buffer_virt = P2V(buffer_phys);
    memset((void*)buffer_virt, 0, 4096);

    uint32_t current_cluster = dir_loc_cluster;

    do {

        //reading data sector wise
        for (uint32_t i = 0; i < sectors_per_cluster; i++){

            //adjusting lba according to the sector number
            uint64_t lba = dir_start_lba + i;
            nvme_read_sector(lba, buffer_phys);

            fat32_directory_entry_t* directory = (fat32_directory_entry_t*) buffer_virt;

            //returns offset at which file name match was found, if not found returns -1
            int file_index = fat32_match_file(directory, file_name, 8);

            // if not -1, then valid match found
            if (file_index != -1){
                pmm_free_page(buffer_phys);
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
        //calcualting the lba for the next cluster
        dir_sector_offset = FILE_SECTOR_OFFSET(current_cluster);
        dir_start_lba = FILE_START_LBA(dir_sector_offset);

    } while (true);

    pmm_free_page(buffer_phys);
    
    k_printf("find file end\n");
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

void fat32_read_file(uint32_t cluster, char* file_name){

    uint64_t buffer_phys = (uint64_t)pmm_request_page();
    uint64_t buffer_virt = P2V(buffer_phys);
    memset((void*)buffer_virt, 0, 4096);
    
    uint64_t file_cluster_size = fat32_find_file(cluster, file_name);
    uint32_t file_cluster = (uint32_t) (file_cluster_size & 0xFFFFFFFF);
    uint32_t file_size = (uint32_t) (file_cluster_size >> 32);
    uint32_t bytes_read = 0;

    if(file_cluster == 0){
        k_printf("File not found!\n");
        return;
    }

    k_printf("\nFAT32: Found File! Reading content...\n");

    k_printf("  -> Content: \"");
    
    uint64_t file_sector_offset = FILE_SECTOR_OFFSET(file_cluster);
    uint64_t file_start_lba = FILE_START_LBA(file_sector_offset);

    while(bytes_read < file_size){

        for(int i = 0; i < sectors_per_cluster; i++){

            uint64_t read_lba = file_start_lba + i;
            nvme_read_sector(read_lba, buffer_phys);


            char* content = (char*)buffer_virt;
            int loop_length;

            if((file_size - bytes_read) <= bytes_per_sector){
                loop_length = (file_size - bytes_read);
            }else{
                loop_length = bytes_per_sector;
            }

            for(int b=0; b < loop_length; b++) {
                k_printf("%c", content[b]);
            }

            bytes_read += loop_length;
            if(bytes_read >= file_size){
                break;
            }
        }

        file_cluster = fat32_find_next_cluster(file_cluster);
        if(file_cluster == EOF){
            break;
        }
        file_sector_offset = FILE_SECTOR_OFFSET(file_cluster);
        file_start_lba = FILE_START_LBA(file_sector_offset);

    }
    

    k_printf("\"\n");

    if(bytes_read != file_size){
        k_printf("ERROR: File read bytes and file size mismatch\n");
    }

    pmm_free_page(buffer_phys);
}

void fat32_set_dir_ent_values(fat32_directory_entry_t* directory, int found_slot, char* file_name, uint8_t attr, uint32_t cluster, uint32_t file_size ){
    
    // copying file name to the directory address
    memcpy(directory[found_slot].name, file_name, 8);
    
    // 0x20 is Archive/File
    directory[found_slot].attributes = attr;
    
    // setting cluster fields
    directory[found_slot].cluster_high = (uint16_t)((cluster >> 16) & 0xFFFF);
    directory[found_slot].cluster_low  = (uint16_t)(cluster & 0xFFFF);
    
    // file size
    directory[found_slot].file_size = file_size;
}


int fat32_add_directory_entry(char* filename, uint32_t directory_loc_cluster, uint32_t file_loc_cluster, uint32_t size, uint8_t attr) {

    uint64_t buffer_phys = (uint64_t)pmm_request_page();
    uint64_t buffer_virt = P2V(buffer_phys);
    memset(buffer_virt, 0, 4096);

    uint32_t dir_cluster = directory_loc_cluster;
    uint32_t dir_sector_offset = FILE_SECTOR_OFFSET(directory_loc_cluster);
    uint64_t dir_start_lba = FILE_START_LBA(dir_sector_offset);

    char empty_marker = 0x00;
    char deleted_marker = 0xE5;

    do {

        //reading data sector wise
        for (uint32_t i = 0; i < sectors_per_cluster; i++){

            //adjusting lba according to the sector number
            uint64_t lba = dir_start_lba + i;
            nvme_read_sector(lba, buffer_phys);

            fat32_directory_entry_t* directory = (fat32_directory_entry_t*) buffer_virt;

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
                nvme_write_sector(lba, buffer_phys);
                pmm_free_page(buffer_phys);
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
                nvme_write_sector(lba, buffer_phys);
                pmm_free_page(buffer_phys);
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
        dir_sector_offset = FILE_SECTOR_OFFSET(dir_cluster);
        dir_start_lba = FILE_START_LBA(dir_sector_offset);

    } while (true);

    pmm_free_page(buffer_phys);
    return -1;
}

void fat32_zero_cluster(uint32_t cluster_number) {
    uint64_t buffer_phys = (uint64_t)pmm_request_page();
    uint64_t buffer_virt = P2V(buffer_phys);
    memset((void*)buffer_virt, 0, 4096); // Clear memory

    uint32_t sector_offset = FILE_SECTOR_OFFSET(cluster_number);
    uint64_t lba = FILE_START_LBA(sector_offset);

    for (uint32_t i = 0; i < sectors_per_cluster; i++) {
        nvme_write_sector(lba + i, buffer_phys);
    }

    pmm_free_page(buffer_phys);
}

void fat32_create_file(char* filename, char* content, int size, uint32_t dir_loc_cluster, uint8_t attr) {

    uint64_t existing_file = fat32_find_file(dir_loc_cluster, filename);

    if (existing_file != 0) {
        k_printf("Error: Name already exists.\n");
        return; 
    }

    uint64_t cluster_size = sectors_per_cluster * bytes_per_sector;
    uint64_t required_clusters = (size + cluster_size - 1) / cluster_size;

    // finding an empty cluster by looking inside the fat table
    uint32_t initial_cluster = fat32_find_free_cluster();
    if (initial_cluster == 0) return;

    k_printf("Allocated Cluster %d for file.\n", initial_cluster);

    // marking it as used inside fat
    fat32_set_fat_entry(initial_cluster, EOF);

    // calculating lba using that cluster value
    uint32_t sector_offset = FILE_SECTOR_OFFSET(initial_cluster);
    uint64_t lba = FILE_START_LBA(sector_offset);

    k_printf("sectors per cluster: %d\n", sectors_per_cluster);
    k_printf("nvme sector size : %d\n", nvme_get_sector_size());
    uint32_t required_sectors = (size / bytes_per_sector) + 1;
    required_sectors = (required_sectors < sectors_per_cluster) ? required_sectors : sectors_per_cluster;
    for(int s = 0; s < required_sectors; s++) {
        k_printf("calculated lba: %d\n", lba + s);
        k_printf("calculated address: %p\n", V2P(content + (s *bytes_per_sector)));
        nvme_write_sector(
            lba + s,
            V2P(content + (s * bytes_per_sector)) // Offset for sector
        );
    }
    k_printf("writing first cluster end\n");

    uint32_t previous_cluster = initial_cluster;

    k_printf("writing further clusters\n");
    for(int i = 1; i < required_clusters; i++){

        // finding an empty cluster by looking inside the fat table
        uint32_t next_free_cluster = fat32_find_free_cluster();
        if (next_free_cluster == 0) return;

        k_printf("Allocated Cluster %d for file.\n", next_free_cluster);

        // marking it as used inside fat
        fat32_set_fat_entry(previous_cluster, next_free_cluster);
        fat32_set_fat_entry(next_free_cluster, EOF);

        // calculating lba using that cluster value
        sector_offset = FILE_SECTOR_OFFSET(next_free_cluster);
        lba = FILE_START_LBA(sector_offset);

        char* current_buffer_pos = content + (i * cluster_size);

        for(int s = 0; s < sectors_per_cluster; s++) {
            nvme_write_sector(
                lba + s, 
                V2P(current_buffer_pos + (s * bytes_per_sector)) // Offset for sector
            );
        }

        previous_cluster = next_free_cluster;

    }
    k_printf("writing further clusters end\n");

    // update the directory
    fat32_add_directory_entry(filename, dir_loc_cluster, initial_cluster, size, attr);
    
    k_printf("File %s written successfully.\n", filename);
}

void fat32_create_dir(char* dirName, uint32_t parent_dir_cluster){

    k_printf("create dir\n");

    uint64_t existing_file = fat32_find_file(parent_dir_cluster, dirName);

    if (existing_file != 0) {
        k_printf("Error: Name already exists.\n");
        return; 
    }

    // finding an empty cluster by looking inside the fat table
    uint32_t initial_cluster = fat32_find_free_cluster();
    if (initial_cluster == 0) return;

    k_printf("Allocated Cluster %d for file.\n", initial_cluster);

    // marking it as used inside fat
    fat32_set_fat_entry(initial_cluster, 0x0FFFFFFF);

    // calculating lba using that cluster value
    uint32_t sector_offset = FILE_SECTOR_OFFSET(initial_cluster);
    uint64_t lba = FILE_START_LBA(sector_offset);

    fat32_add_directory_entry(dirName, parent_dir_cluster, initial_cluster, 0, DIR_ATTR);
    fat32_add_directory_entry(".       ", initial_cluster, initial_cluster, 0, DIR_ATTR);
    fat32_add_directory_entry("..      ", initial_cluster, parent_dir_cluster, 0, DIR_ATTR);

}

void fat32_test_write() {
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

}


void fat32_init(uint64_t partition_start_lba) {
    fat32_read_bpb(partition_start_lba);
    font_renderer_clear_screen();
    // test_impl();
    fat32_test_write();
}