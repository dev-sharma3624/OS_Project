#pragma once
#include <typedefs.h>

#define GPT_SIGNATURE 0x5452415020494645 // in ASCII it is decoded "EFI PART"

typedef struct {
    uint8_t  signature[8]; //if it is a efi system partition it's value will be equal to GPT_SIGNATURE macro
    uint32_t revision;
    uint32_t header_size;
    uint32_t header_crc32;
    uint32_t reserved;
    uint64_t my_lba;
    uint64_t alternate_lba;
    uint64_t first_usable_lba;
    uint64_t last_usable_lba;
    uint8_t  disk_guid[16];
    uint64_t partition_entry_lba; // the lba at which the GUID partition entries array starts
    uint32_t num_partition_entries; // no of entries in the GUID partition array
    uint32_t size_partition_entry; // size of each entry in the GUID partition array
    uint32_t partition_entry_array_crc32;
} __attribute__((packed)) gpt_header_t;

typedef struct {
    uint8_t  type_guid[16]; //uinque id that defines the purpose and type of this partition, 0 value means this partition is inactive/unused
    uint8_t  unique_guid[16];
    uint64_t starting_lba; //the lba at which the partition corresponding to an entry starts
    uint64_t ending_lba; //the lba at which the partition correspondin to an entry ends
    uint64_t attributes;
    uint16_t partition_name[36]; //the name of the partition, UTF-16 coded
} __attribute__((packed)) gpt_entry_t;

void gpt_scan_partition_table(uint32_t nsid);