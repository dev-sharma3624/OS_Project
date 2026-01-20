#include <typedefs.h>
#include <drivers/nvme.h>
#include <drivers/pci.h>
#include <memory_management/paging.h>
#include <libs/k_printf.h>

uint64_t map_nvme(uint64_t physical_addr){

    uint64_t nvme_virt_addr = P2V(physical_addr);

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

void setup_nvme(){

    k_printf("Scanning PCI Bus...\n");
    
    pci_device_info_t nvme = pci_scan_for_device(CLASS_MASS_STORAGE, SUBCLASS_NVME);
    
    if (nvme.found) {
        k_printf("SUCCESS: NVMe Drive Found!\n");
        k_printf(" - Bus: %d, Slot: %d\n", nvme.bus, nvme.slot);
        k_printf(" - Physical BAR Address: %p\n", nvme.physical_address);
    } else {
        k_printf("FAILURE: No NVMe Drive found. Check QEMU flags!\n");
    }

    uint64_t nvme_virt_addr = map_nvme(nvme.physical_address);

    nvme_registers_t* nvme_regs = (nvme_registers_t*)nvme_virt_addr;

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