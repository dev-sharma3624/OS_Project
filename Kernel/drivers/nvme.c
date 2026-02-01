#include <typedefs.h>
#include <drivers/nvme.h>
#include <drivers/pci.h>
#include <memory_management/paging.h>
#include <memory_management/pmm.h>
#include <memory_management/memory.h>
#include <libs/k_printf.h>
#include <libs/k_string.h>

#define NVME_OPCODE_CREATE_IO_SQ  0x01
#define NVME_OPCODE_WRITE 0x01
#define NVME_OPCODE_READ 0x02
#define NVME_OPCODE_CREATE_IO_CQ 0x05
#define NVME_OPCODE_IDENTIFY 0x06

typedef enum {
    ADMIN,
    IO_1
} queue_type_t;

typedef enum {
    CONTROLLER,
    NAMESPACE
} identify_ds_type;

static pci_device_info_t nvme;
static uint32_t stride; //in bytes
static uint32_t sector_size; //in bytes

static nvme_state_t nvme_admin_state;
static nvme_state_t nvme_io_state;
static uint16_t last_cid;

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

void nvme_fetch_device(){

    k_printf("Scanning PCI Bus...\n");
    k_printf("Address of nvme: %p\n", &nvme);

    nvme = pci_scan_for_device(CLASS_MASS_STORAGE, SUBCLASS_NVME);
    
    if (nvme.found) {
        k_printf("SUCCESS: NVMe Drive Found!\n");
        k_printf(" - Bus: %d, Slot: %d\n", nvme.bus, nvme.slot);
        k_printf(" - Physical BAR Address: %p\n", nvme.physical_address);
    } else {
        k_printf("FAILURE: No NVMe Drive found. Check QEMU flags!\n");
    }

}

void nvme_setup_admin_sc_queues(nvme_registers_t* nvme_regs){

    //get physical pages for admin queues
    uint64_t* submission_queue = (uint64_t*) pmm_request_page();
    uint64_t* completion_queue = (uint64_t*) pmm_request_page();

    //clearing the pages
    memset((void*) P2V(submission_queue), 0, 4096);
    memset((void*) P2V(completion_queue), 0, 4096);



    //admin submission queue size
    //one entry in submission queue is 64 bytes => 4096(page size) / 64 = 64 slots
    //asqs is 0 based that is why 63 (0x3F)
    uint32_t asqs = 0x3F;

    nvme_regs->aqa |= asqs;

    //admin completion queue size
    //one entry in completion queue is 16 bytes => 4096 / 16 = 256 slots
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

    nvme_admin_state.sq_virtual_addr = (uint64_t*) P2V(submission_queue);
    nvme_admin_state.cq_virtual_addr = (uint64_t*) P2V(completion_queue);
    nvme_admin_state.sq_tail = 0;
    nvme_admin_state.cq_head = 0;
    nvme_admin_state.sq_size = asqs + 1;
    nvme_admin_state.cq_size = acqs + 1;
    nvme_admin_state.phase = 1;

}

//creates new empty submission queue entry for the admin submission queue
nvme_sqe_t* nvme_create_sqe(queue_type_t q_type){

    nvme_state_t* current_state;
    switch (q_type) {

        case ADMIN:
            current_state = &nvme_admin_state;
            break;
        
        case IO_1:
            current_state = &nvme_io_state;
            break;
        default:
            return;
    }

    uint32_t sq_tail = current_state->sq_tail;
    nvme_sqe_t* sq = (nvme_sqe_t*) current_state->sq_virtual_addr;
    nvme_sqe_t* cmd = &sq[sq_tail];
    memset((void*)cmd, 0, sizeof(nvme_sqe_t));
    return cmd;
}

//notification to controller about new request in admission submission queue
void nvme_ring_sq_doorbell(queue_type_t queue_type){

    nvme_state_t* current_state;
    uint32_t qid;
    switch (queue_type) {
        case ADMIN:
            current_state = &nvme_admin_state;
            qid = 0;
            break;
        
        case IO_1:
            current_state = &nvme_io_state;
            qid = 1;
            break;

        default:
            return;
    }

    current_state->sq_tail = (current_state->sq_tail + 1) % current_state->sq_size;

    uint32_t doorbell_offset = 0x1000 + (2 * qid * stride);
    volatile uint32_t* sq_tail_doorbell = (uint32_t*) (P2V(nvme.physical_address) + doorbell_offset);
    *sq_tail_doorbell = current_state->sq_tail;
}

//reading completion queue entry from admin completion queue
void nvme_completion_poll(queue_type_t queue_type){

    nvme_state_t* current_state;
    uint32_t qid;
    switch (queue_type) {
        case ADMIN:
            current_state = &nvme_admin_state;
            qid = 0;
            break;
        
        case IO_1:
            current_state = &nvme_io_state;
            qid = 1;
            break;

        default:
            return;
    }

    //creating new completion queue entry to read from
    uint32_t cq_head = current_state->cq_head;
    volatile nvme_cqe_t* cq = (volatile nvme_cqe_t*) current_state->cq_virtual_addr;
    volatile nvme_cqe_t* cqe = &cq[cq_head];
    uint16_t current_phase = current_state->phase;

    while(1){

        //waiting for the phase bit to flip to stop polling
        if((cqe->status & 0x01) == current_phase){
            break;
        }

        __asm__ volatile("pause");

    }

    uint16_t status_code = cqe->status >> 1;
    if(status_code != 0){
        k_printf("NVMe Error! Status: %p, Cid: %d, SQ head: %d, CQ head: %d, SQ ID: %d\n", status_code, cqe->cid, cqe->sq_head, current_state->cq_head, cqe->sq_id);
    }

    //incrementin completion queue head
    current_state->cq_head++;
    if(current_state->cq_head >= current_state->cq_size){
        current_state->cq_head = 0;
        current_state->phase = !current_phase;
    }

    //notifying the device that entries till this index have been processed
    uint32_t doorbell_offset = 0x1000 + (((2 * qid) + 1) * stride);
    volatile uint32_t* cq_head_doorbell = (uint32_t*) (P2V(nvme.physical_address) + doorbell_offset);
    *cq_head_doorbell = current_state->cq_head;
    
}

void nvme_setup_io_cq(){

    //gettin physical memory where submission data structure will be written by the driver
    uint64_t io_cq_phy_addr = (uint64_t) pmm_request_page();
    memset((void*) P2V(io_cq_phy_addr), 0, 4096);

    //creating the a new empty submission queue entry
    nvme_sqe_t* cmd = nvme_create_sqe(ADMIN);

    cmd->opcode = NVME_OPCODE_CREATE_IO_CQ;
    cmd->cid = ++last_cid;
    cmd->nsid = 0;
    cmd->prp1 = io_cq_phy_addr;

    uint16_t queue_size = 255; //max number of element that the queue will hold
    uint16_t queue_id = 1;
    cmd->cdw10 = (queue_size << 16) | 1;
    cmd->cdw11 = 1; //physically contiguous block of memory

    nvme_ring_sq_doorbell(ADMIN);

    nvme_completion_poll(ADMIN);

    nvme_io_state.cq_virtual_addr = P2V(io_cq_phy_addr);
    nvme_io_state.cq_head = 0;
    nvme_io_state.cq_size = 256;
    nvme_io_state.phase = 1;

    k_printf("SUCCESS: I/O Completion Queue 1 Created!\n");

}

void nvme_setup_io_sq() {

    //getting physical memory where completion data structure will be written by the controller
    uint64_t io_sq_phy_addr = (uint64_t) pmm_request_page();
    memset((void*) P2V(io_sq_phy_addr), 0, 4096);

    nvme_sqe_t* cmd = nvme_create_sqe(ADMIN);

    cmd->opcode = NVME_OPCODE_CREATE_IO_SQ; // 0x01
    cmd->cid = ++last_cid;
    cmd->nsid = 0;
    cmd->prp1 = io_sq_phy_addr;

    // CDW10: Queue Size (63 for 64 entries) | Queue ID (1)
    uint16_t q_size = 63; 
    uint16_t q_id = 1;
    cmd->cdw10 = (q_size << 16) | q_id;

    // CDW11: Queue Flags & CQ Binding
    // Bits [31:16] = Completion Queue ID to use (We want CQ ID 1 because that's what we setup while setting up completion queue)
    // Bit 0 = 1 (Physically Contiguous)
    uint16_t completion_queue_id = 1;
    cmd->cdw11 = (completion_queue_id << 16) | 1;

    nvme_ring_sq_doorbell(ADMIN);
    nvme_completion_poll(ADMIN);
    
    nvme_io_state.sq_virtual_addr = P2V(io_sq_phy_addr);
    nvme_io_state.sq_tail = 0;
    nvme_io_state.sq_size = 64;
    
    k_printf("SUCCESS: I/O Submission Queue 1 Created!\n");
}

void nvme_setup_io_queue_sizes(nvme_registers_t* nvme_regs){

    nvme_regs->cc &= ~(0xF << 20); // Clear IOCQES
    nvme_regs->cc &= ~(0xF << 16); // Clear IOSQES

    nvme_regs->cc |= (4 << 20);    // Set IOCQES to 4 (16 bytes)
    nvme_regs->cc |= (6 << 16);    // Set IOSQES to 6 (64 bytes)

}

void print_identify_data(nvme_identify_data_t* data) {
    char model[41];  // 40 chars + 1 null
    char serial[21]; // 20 chars + 1 null

    // 1. Manual Loop Copy for Model Number (40 bytes)
    for (int i = 0; i < 40; i++) {
        model[i] = data->mn[i];
    }
    model[40] = '\0'; // Force null terminator

    // 2. Manual Loop Copy for Serial Number (20 bytes)
    for (int i = 0; i < 20; i++) {
        serial[i] = data->sn[i];
    }
    serial[20] = '\0'; // Force null terminator

    // 3. Trim trailing spaces (because NVMe pads with space, not null)
    // We scan backwards from the end. If it's a space, turn it into a null.
    // If it's not a space, we stop (so we don't delete spaces inside the name).
    for (int i = 39; i >= 0; i--) {
        if (model[i] == ' ') model[i] = '\0';
        else break; 
    }

    for (int i = 19; i >= 0; i--) {
        if (serial[i] == ' ') serial[i] = '\0';
        else break;
    }

    k_printf("NVMe Drive Found:\n");
    k_printf("Model: %s\n", model);
    k_printf("Serial: %s\n", serial);
}

void nvme_command_identify(identify_ds_type return_type){

    uint32_t nsid, cdw10;
    switch (return_type){

        case CONTROLLER:
            nsid = 0;
            cdw10 = 1;
            break;
        
        case NAMESPACE:
            nsid = 1;
            cdw10 = 0;
    }

    //location where driver will write the data in memory
    uint64_t buffer = (uint64_t) pmm_request_page();

    //adding a new request to the sumbission queue
    nvme_sqe_t* cmd = nvme_create_sqe(ADMIN);

    cmd->opcode = NVME_OPCODE_IDENTIFY;
    cmd->cid = ++last_cid;
    cmd->nsid = nsid;
    cmd->prp1 = buffer;

    cmd->cdw10 = cdw10;

    //notifying controller about the new request
    nvme_ring_sq_doorbell(ADMIN);

    //polling until phase bit in status code flips
    nvme_completion_poll(ADMIN);

    switch (return_type){

        case CONTROLLER:
            nvme_identify_data_t* identify_data_virt = (nvme_identify_data_t*) P2V(buffer);
            print_identify_data(identify_data_virt);
            break;

        case NAMESPACE:
            nvme_identify_ns_t* identify_ns_virt = (nvme_identify_ns_t*) P2V(buffer);
            uint8_t current_idx = identify_ns_virt->formatted_lba_size & 0x0F;
            uint8_t power_of_2 = identify_ns_virt->lba_formats[current_idx].lba_data_size;
            sector_size = 1 << power_of_2;
            k_printf("Sector size (in bytes): %d\n", sector_size);
            break;

    }

    pmm_free_page(buffer);

}

void nvme_read_sector(uint64_t lba, uint64_t buffer_phy_addr){

    nvme_sqe_t* cmd = nvme_create_sqe(IO_1);

    cmd->opcode = NVME_OPCODE_READ;
    cmd->cid = ++last_cid;
    cmd->nsid = 1;
    cmd->prp1 = buffer_phy_addr;
    cmd->cdw10 = lba & 0xFFFFFFFF;
    cmd->cdw11 = lba >> 32;
    cmd->cdw12 = 0;

    nvme_ring_sq_doorbell(IO_1);
    nvme_completion_poll(IO_1);

}

void nvme_write_sector(uint64_t lba, uint64_t buffer_phy_addr){

    //******************************************************************
    //******************************************************************
    //******************************************************************
    //******************************************************************

    //ADD SAFETY CHECK HERE WHEN TESTING ON REAL DEVICE

    //******************************************************************
    //******************************************************************
    //******************************************************************
    //******************************************************************

    nvme_sqe_t* cmd = nvme_create_sqe(IO_1);

    cmd->opcode = NVME_OPCODE_WRITE;
    cmd->cid = ++last_cid;
    cmd->nsid = 1;
    cmd->prp1 = buffer_phy_addr;
    cmd->cdw10 = lba & 0xFFFFFFFF;
    cmd->cdw11 = lba >> 32;
    cmd->cdw12 = 0;

    nvme_ring_sq_doorbell(IO_1);

    nvme_completion_poll(IO_1);
}

void nvme_test_rw() {

    k_printf("\n[TEST] Starting NVMe Read/Write Test...\n");

    uint64_t phys_addr = (uint64_t)pmm_request_page();
    uint64_t* virt_addr = (uint64_t*)P2V(phys_addr);

    char* msg = "ARCHITECT_WAS_HERE";
    k_strcpy((char*)virt_addr, msg);

    virt_addr[10] = 0xCAFEBABE;
    virt_addr[11] = 0xDEADBEEF;

    k_printf("[TEST] Writing pattern to LBA 5...\n");
    nvme_write_sector(5, phys_addr);
    
    memset(virt_addr, 0, 4096);
    
    k_printf("[TEST] Reading back from LBA 5...\n");
    nvme_read_sector(5, phys_addr);

    if (k_strcmp((char*)virt_addr, msg) == 0 && virt_addr[10] == 0xCAFEBABE) {
        k_printf("[SUCCESS] Data Match! NVMe Driver is OPERATIONAL.\n");
        k_printf("          Read: %s, Magic: %p\n", (char*)virt_addr, virt_addr[10]);
    } else {
        k_printf("[FAILURE] Data Mismatch.\n");
        k_printf("          Expected: %s\n", msg);
        k_printf("          Got:      %s\n", (char*)virt_addr);
    }

    pmm_free_page(phys_addr);
}

void nvme_setup(){

    nvme_fetch_device();

    uint64_t nvme_virt_addr = nvme_map_pages(nvme.physical_address);

    nvme_registers_t* nvme_regs = (nvme_registers_t*)nvme_virt_addr;

    nvme_disable(nvme_regs);

    // Bits 32-35: Doorbell Stride (DSTRD) - How far apart the doorbells are spaced
    uint8_t doorbell_stride = (nvme_regs->cap >> 32) & 0xF;
    stride = (uint32_t) (4 << doorbell_stride);

    nvme_setup_admin_sc_queues(nvme_regs);

    print_nvme_logs1(nvme_regs);

    nvme_command_identify(CONTROLLER);
    nvme_command_identify(NAMESPACE);

    nvme_setup_io_queue_sizes(nvme_regs);

    nvme_setup_io_cq(nvme);
    nvme_setup_io_sq(nvme);

    // nvme_test_rw(nvme);
}

uint32_t nvme_get_sector_size(){
    return sector_size;
}