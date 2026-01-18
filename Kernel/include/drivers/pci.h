#pragma once
#include <typedefs.h>

//port address to which we send our activation command which is the address containing (bus, slot, function, offset)
//so it basically in way establishes connection with a particular device on the PCIe
#define PCI_CONFIG_ADDRESS 0xCF8

//port address to which we read and write data after connection has been established with a device
//we write data here, it goes to the device with established connection
//we read data from here which is sent by the device we're talking to
#define PCI_CONFIG_DATA 0xCFC

//PCI CONFIGURATION SPACE
//Total : 64 bytes
//Represents registers of the device. Every device must have these 64 bytes of information
//that can be used to find, identify, setup devices.
#define PCI_OFFSET_VENDOR_ID 0x00 //2-byte

#define PCI_OFFSET_DEVICE_ID 0x02 //2-byte

#define PCI_OFFSET_COMMAND 0x04 //2-byte, 1st bit allows CPU to read driver's registers, 2nd bit of command represents bus master/DMA enable status (on/off switch)

#define PCI_OFFSET_STATUS 0x06 //2-byte

#define PCI_OFFSET_REVISION_ID 0x08 //1-byte

#define PCI_OFFSET_CLASS 0x0B //2-byte, upper byte: class, lower byte: sub-class

#define PCI_OFFSET_BAR0 0x10 //4-byte, lower 4 bits are flags
                            //bit 3: Prefetchable(1), Non-Prefetchable(0)
                            //bit 1-2: Size => 32-bit(00), 64-bit(10)
                            //bit 0: MMIO(0), I/O(1)
#define PCI_OFFSET_BAR1 0x14 //4-byte


//class codes
#define CLASS_MASS_STORAGE 0x01

//sub-class codes
#define SUBCLASS_NVME 0x08


typedef struct {
    uint8_t bus; //size declared is 8 bits and requires 8 bits. Value ranges from 0-255
    uint8_t slot; //size delcared is 8 bits but requires only 5 bits. Value ranges from 0-32
    uint8_t function; //size declared is 8 bits but requires only 3 bits. Value ranges from 0-7
    uint64_t physical_address;
    uint8_t found;
} pci_device_info_t;

pci_device_info_t pci_scan_for_device(uint8_t class_code, uint8_t subclas_code);
void pci_enable_bus_master(uint8_t bus, uint8_t slot, uint8_t function);