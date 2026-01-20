#pragma once
#include <typedefs.h>

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

void setup_nvme();