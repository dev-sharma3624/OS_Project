#include <Uefi.h>
#include "Simple_File_System_Protocol.h"
#include "Loaded_Image_Protocol.h"
#include "Elf.h"
#include "../boot_info.h"

#define KERNEL_VIRT_BASE 0xFFFFFFFF80000000
#define KERNEL_PHYS_BASE 0x8000000

UINTN mapKey;


void PrintHex(EFI_SYSTEM_TABLE* SystemTable, UINT64 value) {
    CHAR16* hexChars = (CHAR16*)L"0123456789ABCDEF";
    CHAR16 buffer[20];
    
    // We print 16 digits (64-bit)
    for (int i = 15; i >= 0; i--) {
        buffer[i] = hexChars[value % 16];
        value /= 16;
    }
    buffer[16] = 0; // Null terminator
    
    SystemTable->ConOut->OutputString(SystemTable->ConOut, L"0x");
    SystemTable->ConOut->OutputString(SystemTable->ConOut, buffer);
    SystemTable->ConOut->OutputString(SystemTable->ConOut, L"\r\n");
}

EFI_MEMORY_DESCRIPTOR* GetMemoryMap(EFI_SYSTEM_TABLE* systemTable, UINTN* mapSize, UINTN* mapKey, UINTN* descriptorSize, UINT32* descriptorVersion){
    EFI_STATUS status;
    UINTN memMapSize = 0;
    EFI_MEMORY_DESCRIPTOR* memMap = NULL;

    status = systemTable->BootServices->GetMemoryMap(&memMapSize, memMap, mapKey, descriptorSize, descriptorVersion);

    memMapSize += 2 * (*descriptorSize);

    status = systemTable->BootServices->AllocatePool(EfiLoaderData, memMapSize, (void**)&memMap);
    if(status != 0){
        return NULL;
    }

    status = systemTable->BootServices->GetMemoryMap(&memMapSize, memMap, mapKey, descriptorSize, descriptorVersion);
    if(status != 0){
        return NULL;
    }

    *mapSize = memMapSize;
    return memMap;

}

UINT64 LoadKernel(EFI_FILE_PROTOCOL* openFile, EFI_SYSTEM_TABLE* systemTable){
    
    EFI_STATUS status;

    Elf64_Ehdr header;
    UINTN headerSize = sizeof(Elf64_Ehdr);
    status = openFile->Read(openFile, &headerSize, &header);

    if(status != 0 || sizeof(Elf64_Ehdr) != sizeof(header)){
        systemTable->ConOut->OutputString(systemTable->ConOut, L"Error: Could not load LoadedImageProtocol\r\n");
        while(1) { __asm__ __volatile__("hlt"); }
        return 0;
    }

    if(
        header.e_ident[0] != 0x7f ||
        header.e_ident[1] != 'E' ||
        header.e_ident[2] != 'L' ||
        header.e_ident[3] != 'F'
    ){
        systemTable->ConOut->OutputString(systemTable->ConOut, L"Error: File data is corrupt\r\n");
        while(1) { __asm__ __volatile__("hlt"); }
        return 0;
    }

    UINT64 nextHeaderPos = header.e_phoff;

    for(int i = 0; i < header.e_phnum; i++){

        openFile->SetPosition(openFile, nextHeaderPos);

        Elf64_Phdr phdr;
        UINT64 headerSize = header.e_phentsize;
        openFile->Read(openFile, &headerSize, &phdr);

        nextHeaderPos += header.e_phentsize;

        if(phdr.p_type == 1){

            UINTN pagesNeeded = (phdr.p_memsz + 0xFFF) / 0x1000;
            EFI_PHYSICAL_ADDRESS sgmtAdr = phdr.p_vaddr - KERNEL_VIRT_BASE + KERNEL_PHYS_BASE;

            status = systemTable->BootServices->AllocatePages(
                AllocateAddress,
                EfiLoaderData,
                pagesNeeded,
                &sgmtAdr
            );

            if (status != 0) {
                systemTable->ConOut->OutputString(systemTable->ConOut, L"AllocatePages failed!\r\n");
                while(1) { __asm__ __volatile__("hlt"); }
                return 0;
            } else {
                systemTable->ConOut->OutputString(systemTable->ConOut, L"SUCCESS: Allocation worked!\r\n");
            }

            openFile->SetPosition(openFile, phdr.p_offset);

            UINTN fileSize = phdr.p_filesz;
            openFile->Read(openFile, &fileSize, (void*)sgmtAdr);


        }

    }

    return header.e_entry;

}



psf1_font_t* loadFont(EFI_FILE_PROTOCOL* fontFile, EFI_SYSTEM_TABLE* systemTable){
    EFI_STATUS status;

    fontFile->SetPosition(fontFile, 0);

    psf1_header_t* header;
    UINTN headerSize = sizeof(psf1_header_t);
    systemTable->BootServices->AllocatePool(EfiLoaderData, headerSize, (void**)&header);
    fontFile->Read(fontFile, &headerSize, header);

    if(headerSize != sizeof(psf1_header_t)){
        systemTable->ConOut->OutputString(systemTable->ConOut, L"Error: Could not read header of font file\r\n");
        while(1) { __asm__ __volatile__("hlt"); }
    }

    if(header->file_identifier[0] != 0x36 || header->file_identifier[1] != 0x04){
        systemTable->ConOut->OutputString(systemTable->ConOut, L"Error: Font file identifier mismatch\r\n");
        PrintHex(systemTable, header->file_identifier[0]);
        PrintHex(systemTable, header->file_identifier[1]);
        // while(1) { __asm__ __volatile__("hlt"); }
    }else{
        systemTable->ConOut->OutputString(systemTable->ConOut, L"Success: Font file identifier matched\r\n");
        PrintHex(systemTable, header->file_identifier[0]);
        PrintHex(systemTable, header->file_identifier[1]);
    }

    UINTN glyphBufferSize;
    if(header->mode == 1){
        glyphBufferSize = header->char_size * 512;
    }else{
        glyphBufferSize = header->char_size * 256;
    }

    void* glyph_buffer;
    systemTable->BootServices->AllocatePool(EfiLoaderData, glyphBufferSize, (void**)&glyph_buffer);
    fontFile->SetPosition(fontFile, sizeof(psf1_header_t));
    fontFile->Read(fontFile, &glyphBufferSize, glyph_buffer);

    psf1_font_t* font;
    systemTable->BootServices->AllocatePool(EfiLoaderData, sizeof(psf1_font_t), (void**)&font);
    font->header = header;
    font->glyph_buffer = glyph_buffer;

    return font;
}



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

    EFI_STATUS status;

    UINTN mapSize, descriptorSize;
    UINT32 descriptorVersion;

    EFI_GUID gopGuid = EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID;
    EFI_GRAPHICS_OUTPUT_PROTOCOL* gop;

    status = SystemTable->BootServices->LocateProtocol(&gopGuid, 0, (void **)&gop);

    if(status != 0){
        SystemTable->ConOut->OutputString(SystemTable->ConOut, L"Error: Graphics Driver not loading");
        while(1) { __asm__ __volatile__("hlt"); }
    }

    EFI_FILE_PROTOCOL* kernelFile = LoadFile(ImageHandle, SystemTable, L"\\EFI\\BOOT\\kernel.elf");

    if(kernelFile == NULL){
        SystemTable->ConOut->OutputString(SystemTable->ConOut, L"ERROR: Read failed.\r\n");
        while(1) { __asm__ __volatile__("hlt"); }
    }


    UINT64 entryPoint = LoadKernel(kernelFile, SystemTable);

    if (entryPoint == 0) {
        SystemTable->ConOut->OutputString(SystemTable->ConOut, L"FAIL (Invalid ELF)\r\n");
        while(1);
    }

    SystemTable->ConOut->OutputString(SystemTable->ConOut, L"[4] Reading Memory Map...\r\n");

    kernelFile->Close(kernelFile);

    EFI_FILE_PROTOCOL* fontFile = LoadFile(ImageHandle, SystemTable,  L"\\EFI\\BOOT\\zap-light16.psf");

    if(fontFile == NULL){
        SystemTable->ConOut->OutputString(SystemTable->ConOut, L"ERROR: Read failed for font file.\r\n");
        while(1) { __asm__ __volatile__("hlt"); }
    }else{
        SystemTable->ConOut->OutputString(SystemTable->ConOut, L"File read successfully.\r\n");
    }

    psf1_font_t* font = loadFont(fontFile, SystemTable);
    if (font == NULL) {
        SystemTable->ConOut->OutputString(SystemTable->ConOut, L"FAIL (Invalid font)\r\n");
        while(1);
    }else{
        SystemTable->ConOut->OutputString(SystemTable->ConOut, L"Font loaded successfully\r\n");
    }

    fontFile->Close(fontFile);

    
    SystemTable->ConOut->OutputString(SystemTable->ConOut, L"OK\r\n");

    boot_info_t bootInfo;
    bootInfo.frame_buffer.frame_buffer_base = (void*)gop->Mode->FrameBufferBase;
    bootInfo.frame_buffer.frame_buffer_size = gop->Mode->FrameBufferSize;
    bootInfo.frame_buffer.screen_height = gop->Mode->Info->VerticalResolution;
    bootInfo.frame_buffer.screen_width = gop->Mode->Info->HorizontalResolution;
    bootInfo.frame_buffer.pixel_per_scan_line = gop->Mode->Info->PixelsPerScanLine;

    bootInfo.font = font;
    

    typedef void (*KernelStartFunc)(boot_info_t*);
    entryPoint = entryPoint - KERNEL_VIRT_BASE + KERNEL_PHYS_BASE;
    KernelStartFunc kernelStartFunc = (KernelStartFunc)entryPoint;
    

    EFI_MEMORY_DESCRIPTOR* map = GetMemoryMap(SystemTable, &mapSize, &mapKey, &descriptorSize, &descriptorVersion);
    if (map == NULL) {
        SystemTable->ConOut->OutputString(SystemTable->ConOut, L"FAIL (Map Error)\r\n");
        while(1);
    }

    UINTN bufferCapacity = mapSize + (descriptorSize * 2);

    while(1){

        mapSize = bufferCapacity;

        SystemTable->BootServices->GetMemoryMap(&mapSize, map, &mapKey, &descriptorSize, &descriptorVersion);

        bootInfo.m_map = map;
        bootInfo.m_map_size = mapSize;
        bootInfo.m_map_desc_size = descriptorSize;

        status = SystemTable->BootServices->ExitBootServices(ImageHandle, mapKey);

        if (!EFI_ERROR(status)){
            break;
        }
    }

    kernelStartFunc(&bootInfo);

    return 0;

/* 

    ................................................................................................
    ................................................................................................

    This variant was able to load the kernel file, copy it to the primary memory and start running it.
    It was also able to fetch the memory map and pass it's entry point_t to kernel along with the 
    graphics information like FrameBufferBaseAddress and FrameBufferSize.

    As a result we were able to change the color of the screen from the kernel!

    ................................................................................................
    ................................................................................................

    EFI_STATUS status;

    UINTN mapSize, descriptorSize;
    UINT32 descriptorVersion;

    EFI_GUID gopGuid = EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID;
    EFI_GRAPHICS_OUTPUT_PROTOCOL* gop;

    status = SystemTable->BootServices->LocateProtocol(&gopGuid, 0, (void **)&gop);

    if(status != 0){
        SystemTable->ConOut->OutputString(SystemTable->ConOut, L"Error: Graphics Driver not loading");
        while(1) { __asm__ __volatile__("hlt"); }
    }

    EFI_FILE_PROTOCOL* kernelFile = LoadFile(ImageHandle, SystemTable, L"\\EFI\\BOOT\\kernel.elf");

    if(kernelFile == NULL){
        SystemTable->ConOut->OutputString(SystemTable->ConOut, L"ERROR: Read failed.\r\n");
        while(1) { __asm__ __volatile__("hlt"); }
    }


    UINT64 entryPoint = LoadKernel(kernelFile, SystemTable);

    if (entryPoint == 0) {
        SystemTable->ConOut->OutputString(SystemTable->ConOut, L"FAIL (Invalid ELF)\r\n");
        while(1);
    }

    SystemTable->ConOut->OutputString(SystemTable->ConOut, L"[4] Reading Memory Map... ");
    

    EFI_MEMORY_DESCRIPTOR* map = GetMemoryMap(SystemTable, &mapSize, &mapKey, &descriptorSize, &descriptorVersion);
    if (map == NULL) {
        SystemTable->ConOut->OutputString(SystemTable->ConOut, L"FAIL (Map Error)\r\n");
        while(1);
    }
    SystemTable->ConOut->OutputString(SystemTable->ConOut, L"OK\r\n");


    kernelFile->Close(kernelFile);

    boot_info_t bootInfo;
    bootInfo.frame_buffer_base = (void*)gop->Mode->FrameBufferBase;
    bootInfo.frame_buffer_size = gop->Mode->FrameBufferSize;
    bootInfo.screen_height = gop->Mode->Info->VerticalResolution;
    bootInfo.screen_width = gop->Mode->Info->HorizontalResolution;
    bootInfo.pixel_per_scan_line = gop->Mode->Info->PixelsPerScanLine;

    bootInfo.m_map = map;
    bootInfo.m_map_size = mapSize;
    bootInfo.m_map_desc_size = descriptorSize;


    typedef void (*KernelStartFunc)(boot_info_t*);
    KernelStartFunc kernelStartFunc = (KernelStartFunc)entryPoint;

    SystemTable->BootServices->Stall(5000000);

    kernelStartFunc(&bootInfo);

    return 0; */


    /*

    ................................................................................................
    ................................................................................................

    This variant was to test loading and reading a .txt file from disk. This was necessary to validate
    that we had that capability because otherwise it wouldn't be able to load, read and copy the kernel
    .elf file to the RAM before exiting boot services.

    ................................................................................................
    ................................................................................................

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

    return 0; */

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
    UINT64 frame_buffer_size = gop->Mode->FrameBufferSize;


    UINT32* screen = (UINT32*) frameBufferAddr;

    for(UINT64 i = 0; i < frame_buffer_size; i++){
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