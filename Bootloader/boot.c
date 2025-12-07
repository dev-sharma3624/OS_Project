#include <Uefi.h>


EFI_STATUS efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE* SystemTable) {
    /* 

    ................................................................................................
    ................................................................................................

    This variant was to fetch the GRAPHICS_OUTPUT_PROTOCOL using BOOT_SERVICES in EFI_SYSTEM_TABLE.
    It was meant to verify that we were able to get the frame buffer base address along with frame
    buffer size to manipulate the display device at a pixel level.

    ................................................................................................
    ................................................................................................

    SystemTable->ConOut->ClearScreen(SystemTable->ConOut);
    SystemTable->ConOut->OutputString(SystemTable->ConOut, L"Loading graphics driver");

    EFI_GUID gopGuid = EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID;
    EFI_GRAPHICS_OUTPUT_PROTOCOL* gop;
    EFI_STATUS Status;

    Status = SystemTable->BootServices->LocateProtocol(&gopGuid, 0, (void**)&gop);

    if(Status != 0){
        SystemTable->ConOut->OutputString(SystemTable->ConOut, L"Error: Graphics Driver not loading");
        while(1) { __asm__ __volatile__("hlt"); }
    }

    UINT64 frameBufferAddr = gop->Mode->FrameBufferBase;
    UINT64 frameBufferSize = gop->Mode->FrameBufferSize;


    UINT32* screen = (UINT32*) frameBufferAddr;

    for(UINT64 i = 0; i < frameBufferSize; i++){
        screen[i] = 0xFFFF8000;
    }


    while(1) { __asm__ __volatile__("hlt"); }

    return 0; */






    /* 

    ................................................................................................
    ................................................................................................

    This variant was the first executable/runnable code that gave an output. It was meant to verify
    that we were recieving the EFI_SYSTEM_TABLE that would be used for all the operations for loading
    the kernel.

    ................................................................................................
    ................................................................................................

    SystemTable->ConOut->OutputString(SystemTable->ConOut, L"I have the table!\r\n");

    while(1) { 
    }

    return 0; */
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