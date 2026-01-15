bits 64
section .text

; Import the C handler function
extern isr_handler

; Export the symbol so C can access the ISR table (if you generate it here)
; or just export individual ISRs.
global isr_common_stub
global interrupt_return

; =============================================================================
; MACROS: Context Saving & Restoring
; =============================================================================

; 1. SAVE_CONTEXT
; Matches the 'TrapFrame' struct order (reversed, because stack grows down).
; Top of stack currently: [Error Code] [Int No] [RIP] [CS] [RFLAGS] [RSP] [SS]
%macro SAVE_CONTEXT 0
    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push rbp
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15
%endmacro

; 2. RESTORE_CONTEXT
; Pops registers in the reverse order of pushing.
%macro RESTORE_CONTEXT 0
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rbp
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax
%endmacro

; =============================================================================
; MACROS: ISR Entry Points
; =============================================================================

; Macro for Interrupts that DO NOT push an error code (e.g., Timer, Keyboard)
; We push a dummy 0 to keep the stack layout consistent for the TrapFrame.
%macro ISR_NO_ERR 1
global isr%1
isr%1:
    push 0                  ; Push dummy error code
    push %1                 ; Push interrupt number
    jmp isr_common_stub     ; Jump to the heavy lifting
%endmacro

; Macro for Interrupts that DO push an error code (e.g., Page Fault, GPF)
; The CPU has already pushed the error code, so we just push the Int No.
%macro ISR_ERR 1
global isr%1
isr%1:
    ; Error code is already on stack!
    push %1                 ; Push interrupt number
    jmp isr_common_stub
%endmacro

; =============================================================================
; THE COMMON STUB
; =============================================================================

isr_common_stub:
    ; 1. Save all General Purpose Registers
    SAVE_CONTEXT

    ; 2. Prepare for C Function Call (System V AMD64 ABI)
    ; The first argument (RDI) must contain the pointer to our struct.
    ; RSP points to the top of the stack, which IS the start of our TrapFrame.
    mov rdi, rsp

    ; 3. Call the C Handler
    ; void isr_handler(TrapFrame* frame);
    call isr_handler

    ; 4. Restore Context
    ; If the scheduler switched tasks, RSP will be different now!
    RESTORE_CONTEXT

    ; 5. Cleanup Error Code and Int No
    ; We pushed 2 uint64_t values (16 bytes) manually before SAVE_CONTEXT.
    ; We need to remove them so RSP points to the return address (RIP).
    add rsp, 16 

    ; 6. Return from Interrupt
    ; Pops RIP, CS, RFLAGS, RSP, SS automatically.
    iretq

; =============================================================================
; ISR DEFINITIONS (Examples)
; =============================================================================

; Define your specific ISRs here. 
; You will put these addresses into your IDT in C code.

ISR_NO_ERR 6   ; Invalid Opcode Exception
ISR_ERR    13   ; General protection fault
ISR_ERR    14   ; Page Fault
ISR_NO_ERR 32   ; IRQ0 (Timer) - This is the big one for scheduling!
ISR_NO_ERR 33   ; IRQ1 (Keyboard)
ISR_NO_ERR 128 ; System call

interrupt_return:
    ; This function is where a NEW task "wakes up" for the first time.
    ; The stack currently has a TrapFrame on it.
    
    RESTORE_CONTEXT ; Pop R15...RAX
    add rsp, 16     ; Pop Error Code and Int No
    iretq           ; Pop CS, RIP, RFLAGS, SS, RSP -> Start the task!

; Tell the linker we don't need an executable stack
section .note.GNU-stack noalloc noexec nowrite progbits