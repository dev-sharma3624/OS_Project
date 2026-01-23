#pragma once
#include <typedefs.h>

// NVMe register mapping that will be created by pointer casting of BAR address
// this will provide easy read/write to register without manual pointer manipulation
typedef volatile struct {
    uint64_t cap;       // 0x00 - Capabilities
    uint32_t vs;        // 0x08 - Version
    uint32_t intms;     // 0x0C - Interrupt Mask Set
    uint32_t intmc;     // 0x10 - Interrupt Mask Clear
    uint32_t cc;        // 0x14 - Controller Configuration (CRITICAL)
    uint32_t reserved1; // 0x18
    uint32_t csts;      // 0x1C - Controller Status (CRITICAL)
    uint32_t nssr;      // 0x20 - NVM Subsystem Reset
    uint32_t aqa;       // 0x24 - Admin Queue Attributes
    uint64_t asq;       // 0x28 - Admin Submission Queue Base Address
    uint64_t acq;       // 0x30 - Admin Completion Queue Base Address
    uint32_t cmbloc;    // 0x38 - Controller Memory Buffer Location
    uint32_t cmbsz;     // 0x3C - Controller Memory Buffer Size
} __attribute__((packed)) nvme_registers_t;

// to keep track of where we last read/write
typedef struct {
    uint64_t* sq_virtual_addr; // For write commands
    uint64_t* cq_virtual_addr; // For read completions
    uint32_t  sq_tail;         // To track where we are writing
    uint32_t  cq_head;         // To track what we've read
    uint32_t sq_size;          // Max entries supported by submission queue
    uint32_t cq_size;          // Max entries supported by completion queue
    uint8_t phase;       // whether bit flips from 0->1 or 1->0 for a new entry, starts at 1. So for the first time bit flips from 0->1 when a new entry is there.
} nvme_state_t;

// NVMe Submission Queue Entry (64 bytes)
typedef struct {
    uint8_t  opcode;    // Opcode
    uint8_t  flags;     // Fused (00:01) | Reserverd (02:07) | SGL/PRP (08)
    uint16_t cid;       // Command Identifier
    uint32_t nsid;      // Namespace ID
    uint64_t rsvd1;     // Reserved
    uint64_t mptr;      // Metadata Pointer
    uint64_t prp1;      // Physical Region Page 1
    uint64_t prp2;      // Physical Region Page 2 (used if the data transer spans more than one memory page)
    uint32_t cdw10;
    uint32_t cdw11;
    uint32_t cdw12;
    uint32_t cdw13;
    uint32_t cdw14;
    uint32_t cdw15;
} __attribute__((packed)) nvme_sqe_t;

// NVMe Completion Queue Entry (16 bytes)
typedef struct {
    uint32_t cdw0;      // Command-specific result
    uint32_t rsvd;
    uint16_t sq_head;   // Where the SQ Head pointer is currently
    uint16_t sq_id;     // Which Submission Queue this came from
    uint16_t cid;       // Command ID (The ticket number you sent)
    uint16_t status;    // Status Field (bit 0 of status is the phase tag, tells whether an entry is new)
} __attribute__((packed)) nvme_cqe_t;

// Identify Controller Data Structure (4096 bytes)
typedef struct {
    uint16_t vid;          // PCI Vendor ID
    uint16_t ssvid;        // PCI Subsystem Vendor ID
    char     sn[20];       // Serial Number
    char     mn[40];       // Model Number
    char     fr[8];        // Firmware Revision
    // ... rest of the 4096 bytes (power states, capabilities, etc.)
    uint8_t  padding[4024];
} __attribute__((packed)) nvme_identify_data_t;

void nvme_setup();