[bits 64]

global LoadIdt

; void LoadIdt(Idtr* idtrPtr);
LoadIdt:
    lidt [rdi]      ; Load the Interrupt Descriptor Table
    ret
    section .note.GNU-stack noalloc noexec nowrite progbits