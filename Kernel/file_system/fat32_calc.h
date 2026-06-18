#pragma once
#include <typedefs.h>
#include <file_system/fat32.h>

uint16_t fat32_calc_get_bytes_per_sector();
uint64_t fat32_calc_get_fat_size_in_sectors();
uint64_t fat32_calc_get_fat_start_lba();
uint32_t fat32_calc_get_fat_sectors_per_cluster();
void fat32_calc_init_parameters(fat32_bpb_t* bpb, uint64_t partition_start_lba);
void fat32_calc_fat_table_offsets(uint32_t results[], uint32_t cluster_no);
void fat32_calc_dir_offsets(uint32_t results[], uint32_t cluster_no);