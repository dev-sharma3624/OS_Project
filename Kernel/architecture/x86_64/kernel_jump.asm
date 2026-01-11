[bits 64]
[extern kernel_start] ; Function name of the Kernel C function
[global _start]     ; This makes the symbol visible to the Linker

; Define constants
%define PAGE_PRESENT    (1 << 0)
%define PAGE_WRITE      (1 << 1)
%define PAGE_HUGE       (1 << 7)

; Linker Script Constants
%define KERNEL_VIRTUAL_BASE 0xFFFFFFFF80000000
%define KERNEL_PHYSICAL_BASE 0x8000000

section .text
_start:

    ; 1. Stop Interrupts
    ; Just in case UEFI left them enabled, we kill them.
    ; We don't want an interrupt firing while we are changing stacks.
    cli


    ; 2. Setup Page Tables
    ; We are currently running at 0x8000000. 
    ; We must use physical address math for everything right now.

    ; Get address of our static PML4
    mov rax, (BootPML4 - KERNEL_VIRTUAL_BASE + KERNEL_PHYSICAL_BASE)
    
    ; Load CR3 (This activates the map, but we are still executing low)
    mov cr3, rax

    ; Now that the map is active, we can legally talk about 0xFF... addresses.
    ;    We are currently running at low address 0x80xxxxx.
    ;    We MUST jump to the high address 0xFF... before setting up stack.
    mov rax, _higher_half
    jmp rax

_higher_half:

    ; 2. The Stack Switch (The "Squatter" Fix)
    ; We point the Stack Pointer (RSP) to the TOP of our reserved memory.
    ; (Stacks grow downwards, so we start at the top).
    mov rsp, stack_top

    ; 3. The ABI Handshake (Argument Passing)
    ; Your Bootloader (UEFI) put the 'BootInfo*' address into RCX.
    ; Your C Kernel expects the first argument in RDI.
    ; We move it so the data doesn't get lost.
    mov rdi, rcx
    
    ; (Optional: If you use floating point math immediately, 
    ;  we would enable SSE here. For now, it's fine).

    ; 4. Enter the C World
    call kernel_start

    ; 5. The Safety Net
    ; If kernel_start ever returns (it shouldn't), we trap the CPU here.
    cli
    hlt
    jmp $

section .data
align 4096

BootPML4:
    ; Entry 0: Map Low 512GB (Identity) - Keeps us alive right now
    dq (BootPDP_Low - KERNEL_VIRTUAL_BASE + KERNEL_PHYSICAL_BASE) + 0x3
    
    ; Entry 511: Map High 512GB (The Target)
    times 510 dq 0 
    dq (BootPDP_High - KERNEL_VIRTUAL_BASE + KERNEL_PHYSICAL_BASE) + 0x3

BootPDP_Low:
    ; Map 0x0 -> 512GB
    dq (BootPD_Low - KERNEL_VIRTUAL_BASE + KERNEL_PHYSICAL_BASE) + 0x3
    times 511 dq 0

BootPDP_High:
    ; Map the -2GB region
    times 510 dq 0
    dq (BootPD_High - KERNEL_VIRTUAL_BASE + KERNEL_PHYSICAL_BASE) + 0x3
    dq 0

BootPD_Low:
    ; Map 512 entries of 2MB huge pages (1GB Total)
    ; This covers Physical 0x0 to 0x40000000
    %assign i 0
    %rep 512
        dq (i * 0x200000) + 0x83 ; Present + Write + Huge
        %assign i i+1
    %endrep

BootPD_High:
    %assign i 0
    %rep 512
        ; NOTICE: We add KERNEL_PHYSICAL_BASE (0x8000000) here!
        dq (i * 0x200000) + KERNEL_PHYSICAL_BASE + 0x83
        %assign i i+1
    %endrep

section .bss
    ; This is our "Land". We reserve 8KB (8192 bytes) of memory.
    align 16
    stack_bottom:
        resb 8192 ; Reserve 8KB
    stack_top:

; Tell the linker we don't need an executable stack
section .note.GNU-stack noalloc noexec nowrite progbits