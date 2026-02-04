#pragma once
#include <typedefs.h>

#define EOF 0x0FFFFFF8
#define FILE_ATTR 0x20
#define DIR_ATTR 0x10

// BIOS Parameter Block (BPB) for FAT32
typedef struct {
    // --- Common BPB (Offsets 0 - 35) ---
    uint8_t  jmp_boot[3];        // 0x00: Jump Instruction to Boot Code
    uint8_t  oem_name[8];        // 0x03: OEM Name (e.g. "MSDOS5.0")
    uint16_t bytes_per_sector;   // 0x0B: Bytes per Sector (Must be 512, 1024, 2048, or 4096)
    uint8_t  sectors_per_cluster;// 0x0D: Sectors per Cluster (Must be power of 2)
    uint16_t reserved_sectors;   // 0x0E: Sectors Reserved (BPB + FSInfo)
    uint8_t  fat_count;          // 0x10: Number of FATs (Usually 2)
    uint16_t root_dir_entries;   // 0x11: Must be 0 for FAT32
    uint16_t total_sectors_16;   // 0x13: Must be 0 for FAT32
    uint8_t  media_descriptor;   // 0x15: Media Type (0xF8 = Hard Disk)
    uint16_t sectors_per_fat_16; // 0x16: Must be 0 for FAT32
    uint16_t sectors_per_track;  // 0x18: Legacy (Not used on NVMe)
    uint16_t head_count;         // 0x1A: Legacy (Not used on NVMe)
    uint32_t hidden_sectors;     // 0x1C: LBA of Partition Start (Should contain 2048)
    uint32_t total_sectors_32;   // 0x20: Total sectors in volume
    
    // --- FAT32 Specific (Offsets 36 - 90) ---
    uint32_t sectors_per_fat_32; // 0x24: Size of ONE FAT in sectors
    uint16_t ext_flags;          // 0x28: Mirroring flags
    uint16_t fs_version;         // 0x2A: Version (High byte major, Low byte minor)
    uint32_t root_cluster;       // 0x2C: Cluster # of the Root Directory (Usually 2)
    uint16_t fs_info_sector;     // 0x30: Sector # of FSInfo Structure (Usually 1)
    uint16_t backup_boot_sector; // 0x32: Sector # of Backup Boot Sector (Usually 6)
    uint8_t  reserved[12];       // 0x34: Reserved (Must be 0)
    
    // --- Drive Info (Offsets 64 - 90) ---
    uint8_t  drive_number;       // 0x40: BIOS Drive Number (0x80)
    uint8_t  reserved1;          // 0x41: Reserved
    uint8_t  boot_signature;     // 0x42: Extended Boot Signature (0x29)
    uint32_t volume_id;          // 0x43: Volume Serial Number
    char     volume_label[11];   // 0x47: "NO NAME"
    char     fs_type[8];         // 0x52: "FAT32"
    
    // The rest is Boot Code (420 bytes) + Signature (2 bytes)
} __attribute__((packed)) fat32_bpb_t;

// FAT 32 Byte Directory Entry Structure
typedef struct {
    uint8_t  name[8];            // File Name (padded with spaces)
    uint8_t  ext[3];             // Extension (padded with spaces)
    uint8_t  attributes;         // File Attributes (0x10 = Directory, 0x20 = Archive)
    uint8_t  reserved;           // Reserved for Windows NT
    uint8_t  create_time_tenth;  // Creation time (tenths of second)
    uint16_t create_time;        // Creation time (h:m:s)
    uint16_t create_date;        // Creation date (y:m:d)
    uint16_t access_date;        // Last access date
    uint16_t cluster_high;       // High 16 bits of the cluster number
    uint16_t modify_time;        // Last modification time
    uint16_t modify_date;        // Last modification date
    uint16_t cluster_low;        // Low 16 bits of the cluster number
    uint32_t file_size;          // File Size in bytes
} __attribute__((packed)) fat32_directory_entry_t;

void fat32_init(uint64_t partition_start_lba);