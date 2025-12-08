#ifndef ELF_H
#define ELF_H

#include "Structs/Uefi.h"

// -------------------------------------------------------------------------
// 1. Types (Standard ELF Types)
// -------------------------------------------------------------------------
typedef UINT64 Elf64_Addr;
typedef UINT64 Elf64_Off;
typedef UINT16 Elf64_Half;
typedef UINT32 Elf64_Word;
typedef INT32  Elf64_Sword;
typedef UINT64 Elf64_Xword;
typedef INT64  Elf64_Sxword;

// -------------------------------------------------------------------------
// 2. The File Header (The First 64 Bytes)
// -------------------------------------------------------------------------
// This tells us "Is this an ELF file?" and "Where does the code start?"

#define EI_NIDENT 16

typedef struct {
    unsigned char e_ident[EI_NIDENT]; // Magic: 0x7F 'E' 'L' 'F'
    Elf64_Half    e_type;             // Type: 2 = Executable
    Elf64_Half    e_machine;          // Machine: 0x3E = x86-64
    Elf64_Word    e_version;
    Elf64_Addr    e_entry;            // <--- THE ENTRY POINT (Jump here!)
    Elf64_Off     e_phoff;            // <--- Program Header Offset (Where the segments list is)
    Elf64_Off     e_shoff;            // Section Header Offset (We ignore this for now)
    Elf64_Word    e_flags;
    Elf64_Half    e_ehsize;           // Size of this header (should be 64)
    Elf64_Half    e_phentsize;        // Size of one Program Header entry
    Elf64_Half    e_phnum;            // Number of Program Header entries
    Elf64_Half    e_shentsize;
    Elf64_Half    e_shnum;
    Elf64_Half    e_shstrndx;
} Elf64_Ehdr;

// -------------------------------------------------------------------------
// 3. The Program Header (The Segment List)
// -------------------------------------------------------------------------
// This tells us "Copy X bytes from File to Y address in RAM"

typedef struct {
    Elf64_Word    p_type;             // Type: 1 = LOAD (We only care about this one)
    Elf64_Word    p_flags;            // Read/Write/Execute flags
    Elf64_Off     p_offset;           // Offset in the file where data starts
    Elf64_Addr    p_vaddr;            // Virtual Address (Where it goes in RAM)
    Elf64_Addr    p_paddr;            // Physical Address (Ignore)
    Elf64_Xword   p_filesz;           // Size in the file
    Elf64_Xword   p_memsz;            // Size in memory (usually same as filesz)
    Elf64_Xword   p_align;
} Elf64_Phdr;

// Constant to identify "Loadable" segments
#define PT_LOAD 1

#endif