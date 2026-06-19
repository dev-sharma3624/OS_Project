#include <typedefs.h>
#include <file_system/fat32.h>
#include "fat32_calc.h"


uint16_t bytes_per_sector;
uint32_t sectors_per_cluster;
uint64_t data_start_lba;
uint64_t fat_start_lba;
uint64_t fat_size_in_sectors;

#define FAT_BYTES_OFFSET(cluster_no) (cluster_no * 4)
#define FAT_SECTOR_OFFSET(fat_bytes_offset) (fat_bytes_offset / bytes_per_sector)
#define FAT_ENTRY_OFFSET(fat_bytes_offset) (fat_bytes_offset % bytes_per_sector)

#define FILE_SECTOR_OFFSET(cluster_no) ((cluster_no - 2) * sectors_per_cluster)
#define FILE_START_LBA(sector_offset) (sector_offset + data_start_lba)


uint16_t fat32_calc_get_bytes_per_sector(){
    return bytes_per_sector;
}

uint64_t fat32_calc_get_fat_size_in_sectors(){
    return fat_size_in_sectors;
}

uint64_t fat32_calc_get_fat_start_lba(){
    return fat_start_lba;
}

uint32_t fat32_calc_get_fat_sectors_per_cluster(){
    return sectors_per_cluster;
}

void fat32_calc_init_parameters(fat32_bpb_t* bpb, uint64_t partition_start_lba){
    //required for calculating offset
    sectors_per_cluster = bpb->sectors_per_cluster;

    // Calculate Data Area Start
    uint32_t fat_size = bpb->fat_count * bpb->sectors_per_fat_32;
    data_start_lba = partition_start_lba + bpb->reserved_sectors + fat_size;
    bytes_per_sector = bpb->bytes_per_sector;
    fat_size_in_sectors = bpb->sectors_per_fat_32;
    fat_start_lba = partition_start_lba + bpb->reserved_sectors;
}

// fat table sector offset at results[0], entry offset at results[1], lba offset at results[2]
void fat32_calc_fat_table_offsets(uint64_t results[], uint32_t cluster_no){

    uint32_t fat_byte_offset = FAT_BYTES_OFFSET(cluster_no);
    uint32_t fat_sector_offset = FAT_SECTOR_OFFSET(fat_byte_offset);
    uint32_t ent_offset = FAT_ENTRY_OFFSET(fat_byte_offset);

    results[0] = fat_sector_offset;
    results[1] = ent_offset;
    results[2] = fat_start_lba + fat_sector_offset;
}

// dir sector offset at results[0], lba offset at results[1]
void fat32_calc_dir_offsets(uint64_t results[], uint32_t cluster_no){

    //the first lba at which file chain begins in data region
    uint32_t dir_sector_offset = FILE_SECTOR_OFFSET(cluster_no);
    uint64_t dir_start_lba = FILE_START_LBA(dir_sector_offset);

    results[0] = dir_sector_offset;
    results[1] = dir_start_lba;
}