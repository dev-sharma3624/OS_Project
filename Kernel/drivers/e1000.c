#include <typedefs.h>
#include <drivers/e1000_discovery.h>
#include <drivers/e1000.h>
#include <drivers/pci.h>
#include <libs/k_printf.h>
#include <memory_management/paging.h>
#include <memory_management/memory.h>
#include <memory_management/pmm.h>

e1000_device_t e1000;


void e1000_write_register(uint16_t offset, uint32_t value) {
    *(volatile uint32_t*)(e1000.mmio_base + offset) = value;
}

uint32_t e1000_read_register(uint16_t offset) {
    return *(volatile uint32_t*)(e1000.mmio_base + offset);
}

void e1000_clear_mta(){
    for (int i = 0; i < 128; i++) {
        e1000_write_register(E1000_MTA + (i * 4), 0);
    }
}

void e1000_configure_interrupts(){
    e1000_write_register(E1000_IMC, 0xFFFFFFFF); // Disable all
    e1000_read_register(E1000_ICR);              // Clear pending flags
    e1000_write_register(E1000_IMS, IMS_RXT0 | IMS_TXDW | IMS_LSC); // Enable RXT0, TXDW, and LSC
}

void e1000_configure_tctl(){
    uint32_t tctl = 0;
    tctl |= TCTL_EN;  // EN (Enable)
    tctl |= TCTL_PSP;  // PSP (Pad Short Packets)
    tctl |= TCTL_CT_SHIFT;  // CT (Collision Threshold)
    tctl |= TCTL_COLD_SHIFT; // COLD (Collision Distance)

    e1000_write_register(E1000_TCTL, tctl);
}

void e1000_configure_rctl(){
    uint32_t rctl = 0;
    rctl |= RCTL_EN;      // EN (Receive Enable)
    rctl |= RCTL_MPE;      // MPE (Multicast Promiscuous)
    rctl |= RCTL_BAM;     // BAM (Broadcast Accept Mode)
    rctl |= RCTL_BSIZE_2048;

    e1000_write_register(E1000_RCTL, rctl);
}

void e1000_transmit(uint64_t phys_addr, uint16_t length) {

    uint16_t tail = e1000.tx_tail;
    e1000_tx_desc* tx_desc = &e1000.tx_ring[tail];

    tx_desc->addr = phys_addr;
    tx_desc->length = length;

    tx_desc->cmd = CMD_EOP | CMD_IFCS | CMD_RS; 
    
    // 5. Clear old status flags so we can poll it later if needed
    tx_desc->status = 0;
    
    // 6. Advance the software tail (Wrap around at 64)
    e1000.tx_tail = (tail + 1) % 64;
    
    // 7. Ring the Doorbell: Tell the hardware the tail has moved
    e1000_write_register(E1000_TDT, e1000.tx_tail);
}

void e1000_transmission_test(){
    // 1. Allocate a fresh physical page for the packet
    uint64_t test_packet_phys = (uint64_t) pmm_request_page();
    uint8_t* test_packet = (uint8_t*) P2V_DIRECT(test_packet_phys);

    // 2. Destination MAC (Broadcast: FF:FF:FF:FF:FF:FF)
    for(int i = 0; i < 6; i++) test_packet[i] = 0xFF;

    // 3. Source MAC (Your QEMU MAC: 52:54:00:12:34:56)
    test_packet[6]  = 0x52;
    test_packet[7]  = 0x54;
    test_packet[8]  = 0x00;
    test_packet[9]  = 0x12;
    test_packet[10] = 0x34;
    test_packet[11] = 0x56;

    // 4. EtherType (0x88B5 - Experimental)
    test_packet[12] = 0x88;
    test_packet[13] = 0xB5;

    // 5. The Payload
    const char* payload = "PROJECT D OS NETWORK TEST";
    for(int i = 0; payload[i] != '\0'; i++) {
        test_packet[14 + i] = payload[i];
    }

    // Total length: 14 bytes (MAC header) + 25 bytes (String) = 39 bytes.
    // Note: The E1000 PSP bit we set earlier will automatically pad this 
    // to the 64-byte ethernet minimum before pushing it to the wire.

    // 6. Fire the lasers
    k_printf("Transmitting test frame...\n");
    e1000_transmit(test_packet_phys, 39);
}

void e1000_setup(){
    uint64_t tx_ring_buffer = (uint64_t) pmm_request_page();
    if(tx_ring_buffer == 0){
        k_printf("Pmm allocation failed\n");
        return;
    }
    paging_map_page(get_kernel_page_table(), (void*)P2V_DIRECT(tx_ring_buffer), tx_ring_buffer, PT_FLAG_PRESENT | PT_FLAG_READ_WRITE | PT_FLAG_CACHE_DISABLED | PT_FLAG_WRITE_THROUGH, KB_4);
    memset((void*) P2V_DIRECT(tx_ring_buffer), 0, 4096);

    e1000.tx_ring = (e1000_tx_desc*) P2V_DIRECT(tx_ring_buffer);
    e1000.tx_tail = 0;

    uint64_t rx_ring_buffer = (uint64_t) pmm_request_page();
    if(rx_ring_buffer == 0){
        k_printf("Pmm allocation failed\n");
        return;
    }
    paging_map_page(get_kernel_page_table(), (void*)P2V_DIRECT(rx_ring_buffer), rx_ring_buffer, PT_FLAG_PRESENT | PT_FLAG_READ_WRITE | PT_FLAG_CACHE_DISABLED | PT_FLAG_WRITE_THROUGH, KB_4);
    memset((void*) P2V_DIRECT(rx_ring_buffer), 0, 4096);

    e1000_rx_desc* rx_ring = (e1000_rx_desc*) P2V_DIRECT(rx_ring_buffer);
    e1000.rx_ring = rx_ring;
    e1000.rx_tail = 0;

    for(uint64_t i = 0; i < 64; i++){
        uint64_t descriptor_addr = (uint64_t) pmm_request_page();
        if(descriptor_addr == 0){
            k_printf("Pmm allocation failed\n");
            return;
        }
        paging_map_page(get_kernel_page_table(), (void*)P2V_DIRECT(descriptor_addr), descriptor_addr, PT_FLAG_PRESENT | PT_FLAG_READ_WRITE | PT_FLAG_WRITE_THROUGH, KB_4);
        memset((void*) P2V_DIRECT(descriptor_addr), 0, 4096);
        rx_ring[i].addr = descriptor_addr;
        rx_ring[i].status = 0;
    }

    k_printf("E1000: DMA Rings allocated and mapped successfully.\n");

    // Slice the 64-bit physical Tx ring address
    uint32_t tx_low = (uint32_t)(tx_ring_buffer & 0xFFFFFFFF);
    uint32_t tx_high = (uint32_t)(tx_ring_buffer >> 32);

    // Slice the 64-bit physical Rx ring address
    uint32_t rx_low = (uint32_t)(rx_ring_buffer & 0xFFFFFFFF);
    uint32_t rx_high = (uint32_t)(rx_ring_buffer >> 32);

    /* 
    Explanation of constant numbers:

    1024 (for both TDLEN)
    Each entry of e1000_tx_desc is 16 bytes long, even though we've allocated a 4kb page we're only going to use
    1kb from it, that makes 64 entries in total, in future this could be increased to a maximum of 256 entries, for simplicity
    we're not doing it here because, for rx ring we need to provide a buffer for each entry as well, 256 entries means 256 4kb pages
    (one for each entry), we can have different number of entries in tx ring and rx ring but to maintain uniformity, the decision right
    now is to use only 1024 bytes


    0, 0 (for TDH, TDT)
    setting head and tail of ring buffer of transmission ring to 0, initially, when we(the software) transmit we increment the tail,
    when a packet is processed by the hardware, it(the hardware) moves the head, when head == tail, just like in initialization, it depicts,
    there are no packets to process


    0, 64-1 (for RDH, RDT)
    setting head and tail of ring buffer of recieve ring to 0 (head) and 63 (tail, since we're only using 1024 bytes for now, it'll change
    when the lenght changes). We've not set RDT to 0 just like in previous case because rx ring is completely used by the hardware, we only
    tell it what portions of it are currently free, during intialization all indexes (from 0 to 63) can be used, when a packet comes, it's entry is in
    0th index, we'll have to process it, once it is processed, the 0th index can be overwritten, so we set RDT to 0 then, to tell the hardware, after 63 you
    can go upto 0th index, but stop after it because from 1st index, packets are not processed yet
     */
    e1000_write_register(E1000_TDBAL, tx_low);
    e1000_write_register(E1000_TDBAH, tx_high);
    e1000_write_register(E1000_TDLEN, 1024);
    e1000_write_register(E1000_TDH, 0);
    e1000_write_register(E1000_TDT, 0);

    // Tell hardware where the Rx ring is (1024 bytes long)
    e1000_write_register(E1000_RDBAL, rx_low);
    e1000_write_register(E1000_RDBAH, rx_high);
    e1000_write_register(E1000_RDLEN, 1024);
    e1000_write_register(E1000_RDH, 0);
    e1000_write_register(E1000_RDT, 64 - 1); // Give the card all 64 buffers

    k_printf("E1000: DMA Ring addresses loaded into hardware.\n");

    e1000_clear_mta();
    e1000_configure_interrupts();
    e1000_configure_tctl();
    e1000_configure_rctl();

    k_printf("E1000: Device configuration successfull.\n");
}


void e1000_discovery(){
    pci_device_info_t pci_device = pci_scan_for_device(CLASS_NETWORK_CONTROLLER, SUBCLASS_ETHERNET_CONTROLLER);
   
    if (pci_device.found) {
        k_printf("SUCCESS: Network card Found!\n");
        k_printf(" - Bus: %d, Slot: %d\n", pci_device.bus, pci_device.slot);
        k_printf(" - Physical BAR Address: %p\n", pci_device.physical_address);
    } else {
        k_printf("FAILURE: No Network card found. Check QEMU flags!\n");
    }

    for (int i = 0; i < 32; i++) {
        paging_map_page(get_kernel_page_table() ,P2V_DIRECT(pci_device.physical_address) + (i * 0x1000), pci_device.physical_address + (i * 0x1000), PT_FLAG_PRESENT | PT_FLAG_READ_WRITE | PT_FLAG_CACHE_DISABLED, KB_4);
    }

    k_printf("Register Address Low (RAL): %p\n",*(volatile uint32_t*)(P2V_DIRECT(pci_device.physical_address) + 0x5400));
    k_printf("Register Address High (RAH): %p\n",*(volatile uint32_t*)(P2V_DIRECT(pci_device.physical_address) + 0x5404));

    e1000.mmio_base = (uint8_t*) P2V_DIRECT(pci_device.physical_address);

    e1000_setup();
}