#include <typedefs.h>
#include <drivers/nvme.h>
#include <drivers/nvme_interface.h>
#include <memory_management/pmm.h>
#include <memory_management/memory.h>

static uint64_t buffer = 0;
static uint32_t nvme_sector_size; //in bytes

void nvme_interface_initialize_buffer(){
    buffer = (uint64_t) pmm_request_page();
}

uint32_t nvme_interface_get_sector_size(){
    return nvme_sector_size;
}

void nvme_interface_set_sector_size(uint32_t val){
    nvme_sector_size = val;
}

void nvme_interface_read(uint64_t dest, uint64_t lba, uint64_t size){

    if(buffer == 0){
        nvme_interface_initialize_buffer();
    }

    nvme_read_sector(lba, buffer);
    memcpy((void*) dest, P2V_DIRECT(buffer), size);
    
}

void nvme_interface_write(uint64_t data, uint64_t lba, uint64_t size){

    if(buffer == 0){
        nvme_interface_initialize_buffer();
    }

    if (size == nvme_sector_size) {
        memcpy( (void*)P2V_DIRECT(buffer), (void*)data, size);
        nvme_write_sector(lba, buffer);
        return;
    }
    
    nvme_read_sector(lba, buffer);
    memcpy( P2V_DIRECT(buffer), data, size);
    nvme_write_sector(lba, buffer);
}