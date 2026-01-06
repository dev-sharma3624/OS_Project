bits 64
section .text
global switch_task

; void switch_task(TCB* current_task, TCB* next_task)
; RDI = current_task (pointer to the struct where we save the old RSP)
; RSI = next_task    (pointer to the struct where we get the new RSP)

switch_task:
    ; 1. Save Callee-Saved Registers
    ; The C compiler assumes these registers stay the same across function calls.
    ; Since we are "leaving" this function and returning in a different thread,
    ; we must save them manually.
    push rbx
    push rbp
    push r12
    push r13
    push r14
    push r15

    ; 2. Save the Current Stack Pointer
    ; The first member of your TCB struct is 'uint64_t rsp'.
    ; So [rdi] points exactly to that 'rsp' field.
    mov [rdi], rsp

    ; -------------------------------------------------------------
    ; THE CONTEXT SWITCH HAPPENS HERE
    ; -------------------------------------------------------------

    ; 3. Load the New Stack Pointer
    ; We read the stored 'rsp' from the next_task struct.
    mov rsp, [rsi]

    ; 4. Restore Callee-Saved Registers
    ; We are now on the NEW stack. These pop instructions act on the 
    ; data saved when *that* task was last suspended.
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbp
    pop rbx

    ; 5. Return
    ; This 'ret' will pop the return address from the NEW stack.
    ; The CPU jumps to wherever the new task was when it called switch_task.
    ret

; Tell the linker we don't need an executable stack
section .note.GNU-stack noalloc noexec nowrite progbits