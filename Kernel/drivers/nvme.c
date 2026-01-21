#include <typedefs.h>
#include <drivers/nvme.h>
#include <drivers/pci.h>
#include <memory_management/paging.h>
#include <memory_management/pmm.h>
#include <memory_management/memory.h>
#include <libs/k_printf.h>

static nvme_state_t nvme_state;

void print_nvme_logs1(nvme_registers_t* nvme_regs){

    // offset 0 : Capabilities regiester
    uint64_t cap = nvme_regs->cap;

    // Bits 0-15: Max Queue Entries Supported (MQES) - The value is 0-based
    uint16_t max_queue_entries = (cap & 0xFFFF) + 1;
    
    // Bits 24-31: Timeout (TO) in 500ms units
    uint8_t timeout = (cap >> 24) & 0xFF; 

    // Bits 32-35: Doorbell Stride (DSTRD) - How far apart the doorbells are spaced
    uint8_t doorbell_stride = (cap >> 32) & 0xF;

    k_printf("[NVMe] CAP Register Read: %p\n", cap);
    k_printf("Max Queue Size: %d", max_queue_entries);
    k_printf("Timeout limit: %d\n", timeout);;
    k_printf("Doorbell Stride: 2^(%d) bytes\n", 2+doorbell_stride);

    if (cap == 0xFFFFFFFFFFFFFFFF) {
        k_printf("[ERROR] Read -1. Mapping Failed or Device Gone.\n");
    } else if (cap == 0) {
        k_printf("[ERROR] Read 0. Mapping likely pointing to empty RAM.\n");
    } else {
        k_printf("[SUCCESS] We are talking to the NVMe Controller!\n");
    }

}

uint64_t nvme_map_pages(uint64_t physical_addr){

    uint64_t nvme_virt_addr = P2V(physical_addr);

    //mapping 4 pages to have enough space to make up for huge strides between registers
    //and can easily have mulitple doorbels and queues for a multi-core environment
    for(uint64_t i = 0; i < 4; i++){
        paging_map_page(
            get_kernel_page_table(),
            (void*) (nvme_virt_addr + i*4096),
            physical_addr + (i*4096),
            PT_FLAG_PRESENT | PT_FLAG_READ_WRITE | PT_FLAG_WRITE_THROUGH | PT_FLAG_CACHE_DISABLED  
        );
    }

    //flush the TLB for this address so that cpu forgets any old mappings
    __asm__ volatile("invlpg (%0)" :: "r" (nvme_virt_addr));

    return nvme_virt_addr;

}

void nvme_disable(nvme_registers_t* nvme_regs){

    //bit 0 of CC(controlller configuration) register is enabled flag
    if((nvme_regs->cc & 0x01) == 1){ //checking if it is enable or not
        
        k_printf("[NVMe] Controller is currently ENABLED. Resetting...\n");

        nvme_regs->cc &= ~0x01; //disabling the bit if enabled in order to configure the device

        k_printf("[NVMe] Waiting for CSTS.RDY to become 0...\n");

        //when nvme is disabled, the bit 0 (ready bit) of CSTS(controller status) also becomes 0
        //any further operation before the ready bit turns 0 will lead to undefined behaviour
        //until that happens we poll the cpu
        while((nvme_regs->csts & 0x01) == 1){
            __asm__ volatile("pause");
        }
        
        k_printf("[NVMe] Controller Successfully Disabled (Reset).\n");

    }else{
        k_printf("[NVMe] Controller is already disabled.\n");
    }

}

void nvme_fetch_device(pci_device_info_t* nvme){

    k_printf("Scanning PCI Bus...\n");
    
    *nvme = pci_scan_for_device(CLASS_MASS_STORAGE, SUBCLASS_NVME);
    
    if (nvme->found) {
        k_printf("SUCCESS: NVMe Drive Found!\n");
        k_printf(" - Bus: %d, Slot: %d\n", nvme->bus, nvme->slot);
        k_printf(" - Physical BAR Address: %p\n", nvme->physical_address);
    } else {
        k_printf("FAILURE: No NVMe Drive found. Check QEMU flags!\n");
    }

}

void nvme_setup_submission_queues(nvme_registers_t* nvme_regs){

    //get physical pages for queues
    uint64_t* submission_queue = (uint64_t*) pmm_request_page();
    uint64_t* completion_queue = (uint64_t*) pmm_request_page();

    //clearing the pages
    memset((void*) P2V(submission_queue), 0, 4095);
    memset((void*) P2V(completion_queue), 0, 4095);



    //admission submission queue size
    //one entry in submission queue is 64 bytes => 4096(page size) / 64 = 64 slots
    //asqs is 0 based that is why 63 (0x3F)
    uint32_t asqs = 0x3F;

    nvme_regs->aqa |= asqs;

    //admission completion queue size
    //one entry is completion queue is 16 bytes => 4096 / 16 = 256 slots
    //acqs is also 0 based that is why 255 (0xFF)
    uint32_t acqs = 0xFF << 16;

    nvme_regs->aqa |= acqs;


    //providing physical address of pages allocated for queues
    nvme_regs->asq = (uint64_t) submission_queue;
    nvme_regs->acq = (uint64_t) completion_queue;

    //setting css bits (4-6) to 000 (code that we're using NVM command set)
    uint32_t css = (uint32_t) (~(0b111 << 4));
    nvme_regs->cc &= css;

    //setting mps bits (7-10) to 0000 implying we're using 4 KB pages
    uint32_t mps = (~(0xF << 7));
    nvme_regs->cc &= mps;

    //enabling the device by flipping the enable bit (0th bit)
    nvme_regs->cc |= 0x01;

    while((nvme_regs->csts & 0x01) != 1){
        __asm__ volatile("pause");
    }

    nvme_state.sq_virtual_addr = (uint64_t*) P2V(submission_queue);
    nvme_state.cq_virtual_addr = (uint64_t*) P2V(completion_queue);
    nvme_state.sq_tail = 0;
    nvme_state.cq_head = 0;

}

void nvme_setup(){
    
    pci_device_info_t* nvme;

    nvme_fetch_device(nvme);

    uint64_t nvme_virt_addr = nvme_map_pages(nvme->physical_address);

    nvme_registers_t* nvme_regs = (nvme_registers_t*)nvme_virt_addr;

    nvme_disable(nvme_regs);

    nvme_setup_submission_queues(nvme_regs);

    print_nvme_logs1(nvme_regs);
}