#include <typedefs.h>
#include "fat32_calc.h"
#include <drivers/nvme_interface.h>
#include "ft_nvm_bridge.h"

typedef enum{
    NVME,
    FAT
} MINIMUM_SECTOR_SIZE;

MINIMUM_SECTOR_SIZE min_sector_size;
uint32_t min_bytes_per_sector = 0;
bool force_read_allowed = true;

void ft_nvm_bridge_find_min_bytes_per_sector(){
    uint16_t fat_sector_size = fat32_calc_get_bytes_per_sector(); 
    min_bytes_per_sector = nvme_interface_get_sector_size();
    min_sector_size = NVME;
    
    if(fat_sector_size < min_bytes_per_sector){
        min_sector_size = FAT;
        min_bytes_per_sector =  fat_sector_size;
    }

}

void ft_nvm_bridge_read(uint64_t dest, uint64_t lba, uint64_t size){

    if (min_bytes_per_sector == 0) {
        ft_nvm_bridge_find_min_bytes_per_sector();
    }

    if (min_sector_size == FAT) {
        nvme_interface_read(dest, lba, size);
        return;
    }

    uint64_t remaining_size = size;
    uint64_t current_lba = lba;
    uint64_t current_dest = dest;

    while (remaining_size > 0) {
        uint64_t chunk_size = min_bytes_per_sector;
        if (remaining_size < min_bytes_per_sector) {
            chunk_size = remaining_size;
        }

        nvme_interface_read(current_dest, current_lba, chunk_size);

        current_dest += chunk_size;
        current_lba++;
        remaining_size -= chunk_size;
    }

}

void ft_nvm_bridge_write(uint64_t data, uint64_t lba, uint64_t size){

    if(min_bytes_per_sector == 0){
        ft_nvm_bridge_find_min_bytes_per_sector();
    }

    if(min_sector_size == FAT){
        nvme_interface_write(data, lba, size);
        return;
    }
    
    uint64_t remaining_size = size;
    uint64_t current_lba = lba;
    uint64_t current_data = data;

    while (remaining_size > 0) {
        uint64_t chunk_size = min_bytes_per_sector;
        if (remaining_size < min_bytes_per_sector) {
            chunk_size = remaining_size;
        }

        nvme_interface_write(current_data, current_lba, chunk_size);

        current_data += chunk_size;
        current_lba++;
        remaining_size -= chunk_size;
    }
}

void ft_nvm_bridge_force_read(uint64_t dest, uint64_t lba, uint64_t size){

    if (!force_read_allowed) {
        return;
    }

    if (min_bytes_per_sector == 0) {
        ft_nvm_bridge_find_min_bytes_per_sector();
    }

    if (min_sector_size == FAT) {
        nvme_interface_read(dest, lba, size);
        return;
    }

    uint64_t remaining_size = size;
    uint64_t current_lba = lba;
    uint64_t current_dest = dest;

    while (remaining_size > 0) {
        uint64_t chunk_size = min_bytes_per_sector;
        if (remaining_size < min_bytes_per_sector) {
            chunk_size = remaining_size;
        }

        nvme_interface_read(current_dest, current_lba, chunk_size);

        current_dest += chunk_size;
        current_lba++;
        remaining_size -= chunk_size;
    }
}