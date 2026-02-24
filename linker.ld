ENTRY(_start)
OUTPUT_FORMAT(elf64-x86-64)

SECTIONS
{
    /* 1. Set the Virtual Address (High) */
    . = 0xFFFFFFFF80000000;

    /* 2. Set our physical starting point (Low) */
    /* We use this variable 'phys' to track where we are in Physical RAM */
    phys = 0x8000000;

    _KernelStart = .;

    /* 3. Text Section */
    /* AT(phys) tells linker to store data at 0x8000000 */
    .text : AT(phys)
    {
        *(.text)
    }
    
    /* Update Virtual cursor to next page */
    . = ALIGN(4096);
    /* Update Physical cursor to match (Current Phys + Size of Text, aligned) */
    phys = phys + SIZEOF(.text);
    phys = ALIGN(phys, 4096);

    /* 4. Read-Only Data */
    .rodata : AT(phys)
    {
        *(.rodata)
        *(.rodata.*) 
        *(.eh_frame)    
    }

    . = ALIGN(4096);
    phys = phys + SIZEOF(.rodata);
    phys = ALIGN(phys, 4096);

    /* 5. Read-Write Data */
    .data : AT(phys)
    {
        *(.data)
    }

    . = ALIGN(4096);
    phys = phys + SIZEOF(.data);
    phys = ALIGN(phys, 4096);

    /* 6. BSS */
    .bss : AT(phys)
    {
        *(.bss)
        *(COMMON)
    }

    _KernelEnd = .;
}