#include <typedefs.h>
#include <drivers/pci.h>
#include <architecture/x86_64/io.h>

//creates the final address for setup
uint32_t pci_create_address(uint8_t bus, uint8_t slot, uint8_t function, uint8_t offset){
    //converting to larger data size so that they can be appended together to make the final address
    uint32_t lbus = (uint32_t) bus;
    uint32_t lslot = (uint32_t) slot;
    uint32_t lfunction = (uint32_t) function;

    uint32_t address = 0x80000000 | //bit 31 must be 1 otherwise transaction is ignored
    lbus << 16 | //8-bit bus value must be in bit 16-23 of final address
    lslot << 11 | //5-bit slot value must be in 11-15
    lfunction << 8 | //3-bit function value must be in bit 8-10
    offset & 0xFC; //clearing bit 0 and 1 since PCI registers are always 4-bytes aligned

    return address;
}

uint16_t pci_read_word(uint8_t bus, uint8_t slot, uint8_t function, uint8_t offset){
    uint32_t address = pci_create_address(bus, slot, function, offset);

    io_out_l(PCI_CONFIG_ADDRESS, address); //establishing connnection

    uint32_t raw_data = io_in_l(PCI_CONFIG_DATA);

    //register size of every hardware device on PCIe are 4-bytes aligned, i.e., size of every register is 32 bits so it always gives a 32-bit value
    // AND with 2 of any 2-byte aligned offset (0x00, 0x02, 0x04) will either give either 0 or 2
    // 0 means we need lower 2 bytes, 2 means we need upper 2 bytes
    //if lower 2 bytes, & 0xFFFF automatically cleans the upper two bytes
    //if upper 2 bytes, right shift them (2 * 8 = 16) to lower 2 bytes
    return (uint16_t) (raw_data >> ((offset & 2) * 8)) & 0xFFFF;
}

//same as pci_read_word but this returns the full 4-byte value
uint32_t pci_read_dword(uint8_t bus, uint8_t slot, uint8_t function, uint8_t offset){
    
    uint32_t address = pci_create_address(bus, slot, function, offset);

    io_out_b(PCI_CONFIG_ADDRESS, address); //establishing connnection

    return io_in_l(PCI_CONFIG_DATA);
}

void pci_write_word(uint8_t bus, uint8_t slot, uint8_t function, uint8_t offset, uint16_t value){
    uint32_t address = pci_create_address(bus, slot, function, offset);

    io_out_l(PCI_CONFIG_ADDRESS, address);
    io_out_l(address, value);
}

pci_device_info_t pci_scan_for_device(uint8_t class_code, uint8_t subclas_code){
    pci_device_info_t device = {0};

    for(uint16_t bus = 0; bus < 256; bus++){

        for(uint8_t slot = 0; slot < 32; slot++){

            for(uint8_t func  = 0; func < 8; func++){

                uint16_t vendor_id = pci_read_word((uint8_t) bus, slot, func, PCI_OFFSET_VENDOR_ID);

                if(vendor_id == 0xFFFF) continue;

                uint16_t device_id = pci_read_word(bus, slot, func, PCI_OFFSET_DEVICE_ID);
                uint16_t class_subclass = pci_read_word((uint8_t) bus, slot, func, PCI_OFFSET_CLASS);
                uint8_t class = (class_subclass >>  8) & 0xFF;
                uint8_t sub_class = class_subclass & 0xFF;

                if(class == class_code && sub_class == subclas_code){
                    uint64_t bar0 = pci_read_dword((uint8_t) bus, slot, func, PCI_OFFSET_BAR0);
                    uint64_t bar1 = 0x00; //initializing to 0 as we have to check first whether we need bar1 or not

                    if(bar0 & 0x06){ //checking value of bits 1 and 2
                        bar1 = pci_read_dword((uint8_t) bus, slot, func, PCI_OFFSET_BAR1);
                    }

                    uint64_t final_address = bar1 << 32 | bar0 & 0xFFFFFFF0; //clearing lower 4 bits of bar0 as they're flags and not address
                    
                    device.bus = bus;
                    device.slot = slot;
                    device.function = func;
                    device.physical_address = final_address;
                    device.found = 1;

                    pci_enable_bus_master((uint8_t) bus, slot, func);

                    return device;
                }

            }

        }

    }
    
    return device;
}


void pci_enable_bus_master(uint8_t bus, uint8_t slot, uint8_t function){
    uint16_t command = pci_read_word(bus, slot, function, PCI_OFFSET_COMMAND);

    if(!(command & 0x02)){ //2 bit of command tells whether DMA/MMIO is enabled or not
        command |= 0x02; //enabling if not
    }

    pci_write_word(bus, slot, function, PCI_OFFSET_COMMAND, command);
}