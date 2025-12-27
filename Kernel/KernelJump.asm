[bits 64]
[extern kernelStart] ; Function name of the Kernel C function
[global _start]     ; This makes the symbol visible to the Linker

section .text
_start:
    ; 1. Stop Interrupts
    ; Just in case UEFI left them enabled, we kill them.
    ; We don't want an interrupt firing while we are changing stacks.
    cli

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
    call kernelStart

    ; 5. The Safety Net
    ; If kernelStart ever returns (it shouldn't), we trap the CPU here.
    cli
    hlt
    jmp $

section .bss
    ; This is our "Land". We reserve 8KB (8192 bytes) of memory.
    align 16
    stack_bottom:
        resb 8192 ; Reserve 8KB
    stack_top:

; Tell the linker we don't need an executable stack
section .note.GNU-stack noalloc noexec nowrite progbits