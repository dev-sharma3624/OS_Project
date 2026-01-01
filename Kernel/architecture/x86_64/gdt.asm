[bits 64]           ; "Hey assembler, generate 64-bit machine code."
global _LoadGDT      ; "Make this function visible to C files."

_LoadGDT:
    ; --- STEP 1: Tell CPU where the table is ---
    ; In C, you called _LoadGDT(&gdt_descriptor). 
    ; The 'GDTDesc' pointer is passed in the register RDI.
    lgdt [rdi]      ; "Load Global Descriptor Table from the address in RDI."
                    ; Now the CPU knows WHERE the table is.

    ; --- STEP 2: Update Data Bookmarks ---
    ; We want DS, ES, FS, GS, SS to point to offset 0x10 (Kernel Data).
    ; We can't move numbers directly into segment registers, 
    ; so we put 0x10 into a normal register (AX) first.
    mov ax, 0x10    
    
    mov ds, ax      ; "Set Data Segment to 0x10"
    mov es, ax      ; "Set Extra Segment to 0x10"
    mov fs, ax      ; "Set FS Segment to 0x10"
    mov gs, ax      ; "Set GS Segment to 0x10"
    mov ss, ax      ; "Set Stack Segment to 0x10"

    ; --- STEP 3: Update Code Bookmark (The Tricky Part) ---
    ; We want CS to point to 0x08 (Kernel Code).
    ; PROBLEM: You cannot simply type 'mov cs, 0x08'. The CPU forbids it.
    ; WHY? Changing CS changes how the CPU reads the *next* instruction.
    ; If you mess it up, the CPU doesn't know what to execute next.
    
    ; TRICK: We fake a "Return".
    ; When you return from a function, the CPU pops an Address and a Segment 
    ; from the stack to know where to go back to.
    ; We will manually push a fake "return address" and our DESIRED Segment (0x08)
    ; onto the stack, then tell the CPU to "Return".

    pop rdi         ; Grab the "real" return address (back to C) and save it in RDI.
    
    mov rax, 0x08   ; Prepare the value 0x08.
    push rax        ; Push 0x08 onto the stack (This will become the new CS).
    
    push rdi        ; Push the return address back onto the stack.
    
    ; Stack now looks like: [ 0x08 (New CS) ] [ Return Address ]
    
    retfq           ; "Return Far Quad"
                    ; This instruction does two things atomically:
                    ; 1. Pops the top value into Instruction Pointer (RIP).
                    ; 2. Pops the NEXT value into Code Segment (CS).
                    ; RESULT: We are now running at 'Return Address' with CS = 0x08!

    ; Add this line at the bottom to silence the linker warning
    section .note.GNU-stack noalloc noexec nowrite progbits