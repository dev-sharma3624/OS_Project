#pragma once
#include <typedefs.h>

void ft_nvm_bridge_find_min_bytes_per_sector();
void ft_nvm_bridge_read(uint64_t dest, uint64_t lba, uint64_t size);
void ft_nvm_bridge_write(uint64_t data, uint64_t lba, uint64_t size);
void ft_nvm_bridge_force_read(uint64_t dest, uint64_t lba, uint64_t size);