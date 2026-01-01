[bits 64]

global _LoadIdt

; void _LoadIdt(idtr_t* idt_ptr);
_LoadIdt:
    lidt [rdi]      ; Load the Interrupt Descriptor Table
    ret
    section .note.GNU-stack noalloc noexec nowrite progbits