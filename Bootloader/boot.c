#include <Uefi.h>


EFI_STATUS efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE* SystemTable) {
    
    SystemTable->ConOut->OutputString(SystemTable->ConOut, L"I have the table!\r\n");

    while(1) { 
    }

    return 0;
}

/* 

clang -target x86_64-pc-win32-coff \
      -ffreestanding \
      -fno-stack-protector \
      -fshort-wchar \
      -mno-red-zone \
      -I Bootloader/Structs \
      -c Bootloader/boot.c -o boot.o



lld-link /subsystem:efi_application \
         /entry:efi_main \
         /out:image/EFI/BOOT/BOOTX64.EFI \
         boot.o


qemu-system-x86_64 -bios /usr/share/ovmf/OVMF.fd -drive format=raw,file=fat:rw:image

 */