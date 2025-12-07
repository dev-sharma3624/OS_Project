#include <Uefi.h>
#include "Simple_File_System_Protocol.h"
#include "Loaded_Image_Protocol.h"


EFI_FILE_PROTOCOL* LoadFile(EFI_HANDLE handle, EFI_SYSTEM_TABLE* systemTable, CHAR16* path){
    EFI_STATUS status;

    EFI_GUID lop = EFI_LOADED_IMAGE_PROTOCOL_GUID;
    EFI_LOADED_IMAGE_PROTOCOL* loadedImage;

    status = systemTable->BootServices->HandleProtocol(handle, &lop, (void**)&loadedImage);

    if (status != 0) {
        systemTable->ConOut->OutputString(systemTable->ConOut, L"Error: Could not load LoadedImageProtocol\r\n");
        while(1) { __asm__ __volatile__("hlt"); }
        return NULL;
    }

    EFI_GUID fs = EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID;
    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *fileProtocol;

    status = systemTable->BootServices->HandleProtocol(
        loadedImage->DeviceHandle,
        &fs,
        (void**)&fileProtocol
    );

    if (status != 0) {
        systemTable->ConOut->OutputString(systemTable->ConOut, L"Error: Could not load FileSystemProtocol\r\n");
        while(1) { __asm__ __volatile__("hlt"); }
        return NULL;
    }

    EFI_FILE_PROTOCOL *root;
    status = fileProtocol->OpenVolume(fileProtocol, &root);


    if (status != 0) {
        systemTable->ConOut->OutputString(systemTable->ConOut, L"Error: Could not open root directory\r\n");
        while(1) { __asm__ __volatile__("hlt"); }
        return NULL;
    }

    EFI_FILE_PROTOCOL *fileHandle;
    status = root->Open(
        root,
        &fileHandle,
        path,
        EFI_FILE_MODE_READ,
        0
    );

    if (status != 0) {
        systemTable->ConOut->OutputString(systemTable->ConOut, L"Error: Could not find file\r\n");
        while(1) { __asm__ __volatile__("hlt"); }
        return NULL;
    }

    return fileHandle;

}


EFI_STATUS efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE* SystemTable) {

    EFI_FILE_PROTOCOL* kernelFile = LoadFile(ImageHandle, SystemTable, L"\\EFI\\BOOT\\kernel.txt");

    if(kernelFile != NULL){
        SystemTable->ConOut->OutputString(SystemTable->ConOut, L"SUCCESS: Found kernel.txt! Reading...\r\n");

        UINTN bufferSize = 100;
        char buffer[101];

        EFI_STATUS status =  kernelFile->Read(kernelFile, &bufferSize, buffer);

        if(status == 0){
            buffer[bufferSize] = 0;

            SystemTable->ConOut->OutputString(SystemTable->ConOut, L"File Contents: ");

            for(UINTN i = 0; i < bufferSize; i++){
                CHAR16 string[2];

                string[0] = (CHAR16)buffer[i];
                string[1] = 0;

                SystemTable->ConOut->OutputString(SystemTable->ConOut, string);

            }
            SystemTable->ConOut->OutputString(SystemTable->ConOut, L"\r\n");
            while(1) { __asm__ __volatile__("hlt"); }
        }else {
            SystemTable->ConOut->OutputString(SystemTable->ConOut, L"ERROR: Read failed.\r\n");
            while(1) { __asm__ __volatile__("hlt"); }
        }

        kernelFile->Close(kernelFile);

    }

    return 0;

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