#pragma once
#include <typedefs.h>

uint32_t nvme_interface_get_sector_size();
void nvme_interface_set_sector_size(uint32_t val);
void nvme_interface_read(uint64_t dest, uint64_t lba, uint64_t size);
void nvme_interface_write(uint64_t data, uint64_t lba, uint64_t size);