#!/bin/bash

# Exit immediately if any command fails
set -e

echo "=> Compiling Kernel C source files..."
gcc -ffreestanding -mno-red-zone -mgeneral-regs-only -I Kernel/include -c Kernel/main/kernel.c -o image/object/kernel.o
gcc -ffreestanding -mno-red-zone -mgeneral-regs-only -I Kernel/include -c Kernel/main/shell.c -o image/object/shell.o
gcc -ffreestanding -mno-red-zone -mgeneral-regs-only -I Kernel/include -c Kernel/drivers/font_renderer.c -o image/object/font_renderer.o
gcc -ffreestanding -mno-red-zone -mgeneral-regs-only -I Kernel/include -c Kernel/libs/k_printf.c -o image/object/k_printf.o
gcc -ffreestanding -mno-red-zone -mgeneral-regs-only -I Kernel/include -c Kernel/libs/k_string.c -o image/object/k_string.o
gcc -ffreestanding -mno-red-zone -mgeneral-regs-only -I Kernel/include -c Kernel/architecture/x86_64/gdt.c -o image/object/gdt.o
gcc -ffreestanding -mno-red-zone -mgeneral-regs-only -I Kernel/include -c Kernel/architecture/x86_64/idt.c -o image/object/idt.o
gcc -ffreestanding -mno-red-zone -mgeneral-regs-only -I Kernel/include -c Kernel/architecture/x86_64/io.c -o image/object/io.o
gcc -ffreestanding -mno-red-zone -mgeneral-regs-only -I Kernel/include -c Kernel/architecture/x86_64/pic.c -o image/object/pic.o
gcc -ffreestanding -mno-red-zone -mgeneral-regs-only -I Kernel/include -c Kernel/drivers/keyboard_driver.c -o image/object/keyboard_driver.o
gcc -ffreestanding -mno-red-zone -mgeneral-regs-only -I Kernel/include -c Kernel/architecture/x86_64/page_fault.c -o image/object/page_fault.o
gcc -ffreestanding -mno-red-zone -mgeneral-regs-only -I Kernel/include -c Kernel/architecture/x86_64/gen_prot_fault.c -o image/object/gen_prot_fault.o
gcc -ffreestanding -mno-red-zone -mgeneral-regs-only -I Kernel/include -c Kernel/memory_management/memory.c -o image/object/memory.o
gcc -ffreestanding -mno-red-zone -mgeneral-regs-only -I Kernel/include -c Kernel/memory_management/m_bitmap.c -o image/object/m_bitmap.o
gcc -ffreestanding -mno-red-zone -mgeneral-regs-only -I Kernel/include -c Kernel/memory_management/pmm.c -o image/object/pmm.o
gcc -ffreestanding -mno-red-zone -mgeneral-regs-only -I Kernel/include -c Kernel/memory_management/paging.c -o image/object/paging.o
gcc -ffreestanding -mno-red-zone -mgeneral-regs-only -I Kernel/include -c Kernel/memory_management/heap.c -o image/object/heap.o
gcc -ffreestanding -mno-red-zone -mgeneral-regs-only -I Kernel/include -c Kernel/drivers/timer.c -o image/object/timer.o
gcc -ffreestanding -mno-red-zone -mgeneral-regs-only -I Kernel/include -c Kernel/cpu_scheduling/process.c -o image/object/process.o
gcc -ffreestanding -mno-red-zone -mgeneral-regs-only -I Kernel/include -c Kernel/cpu_scheduling/scheduler.c -o image/object/scheduler.o
gcc -ffreestanding -mno-red-zone -mgeneral-regs-only -I Kernel/include -c Kernel/cpu_scheduling/push_pop_cli.c -o image/object/push_pop_cli.o
gcc -ffreestanding -mno-red-zone -mgeneral-regs-only -I Kernel/include -c Kernel/architecture/x86_64/syscall_dispatcher.c -o image/object/syscall_dispatcher.o
gcc -ffreestanding -mno-red-zone -mgeneral-regs-only -I Kernel/include -c Kernel/architecture/x86_64/syscalls.c -o image/object/syscalls.o
gcc -ffreestanding -mno-red-zone -mgeneral-regs-only -I Kernel/include -c Kernel/drivers/pci.c -o image/object/pci.o
gcc -ffreestanding -mno-red-zone -mgeneral-regs-only -I Kernel/include -c Kernel/drivers/nvme.c -o image/object/nvme.o
gcc -ffreestanding -mno-red-zone -mgeneral-regs-only -I Kernel/include -c Kernel/file_system/gpt.c -o image/object/gpt.o
gcc -ffreestanding -mno-red-zone -mgeneral-regs-only -I Kernel/include -c Kernel/file_system/fat32.c -o image/object/fat32.o
gcc -ffreestanding -mno-red-zone -mgeneral-regs-only -I Kernel/include -c Kernel/file_system/fs_interface.c -o image/object/fs_interface.o

echo "=> Assembling hardware & context-switching ASM files..."
nasm -f elf64 Kernel/architecture/x86_64/kernel_jump.asm -o image/object/kernel_jump_asm.o
nasm -f elf64 Kernel/architecture/x86_64/gdt.asm -o image/object/gdt_asm.o
nasm -f elf64 Kernel/architecture/x86_64/idt.asm -o image/object/idt_asm.o
nasm -f elf64 Kernel/architecture/x86_64/interrupts.asm -o image/object/interrupts_asm.o
nasm -f elf64 Kernel/architecture/x86_64/switch.asm -o image/object/switch_asm.o

echo "=> Linking Object files into Kernel ELF..."
ld -T image/linker.ld -o image/EFI/BOOT/kernel.elf image/object/kernel_jump_asm.o image/object/kernel.o image/object/font_renderer.o \
      image/object/k_printf.o image/object/gdt.o image/object/gdt_asm.o image/object/idt.o image/object/idt_asm.o \
      image/object/io.o image/object/pic.o image/object/keyboard_driver.o image/object/page_fault.o image/object/gen_prot_fault.o image/object/memory.o \
      image/object/m_bitmap.o image/object/pmm.o image/object/paging.o image/object/heap.o image/object/timer.o \
      image/object/interrupts_asm.o image/object/process.o image/object/scheduler.o image/object/switch_asm.o \
      image/object/push_pop_cli.o image/object/syscall_dispatcher.o image/object/pci.o image/object/nvme.o \
      image/object/k_string.o image/object/gpt.o image/object/fat32.o image/object/syscalls.o image/object/shell.o \
      image/object/fs_interface.o

echo "=> Build Complete! kernel.elf is ready."