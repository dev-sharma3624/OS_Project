#include "Typedefs.h"
#include "../boot_info.h"
#include "Memory.h"
#include "MemoryBitmap.h"
#include "MemoryDescriptor.h"

static MemoryBitmap bitmap;
extern uint64_t _KernelStart;
extern uint64_t _KernelEnd;
uint64_t freeMemory;
uint64_t reservedMemory;
uint64_t usedMemory;
bool initialized = false;


#define PAGE_SIZE 4096
#define RESERVE_MEMORY() (freeMemory -= PAGE_SIZE,  reservedMemory += PAGE_SIZE)
#define UNRESERVE_MEMORY() (freeMemory += PAGE_SIZE, reservedMemory -= PAGE_SIZE)

void LockPages(void* address, uint64_t count);
void UnlockPages(void* address, uint64_t count);


void InitPageFrameAllocator(BOOT_INFO* bootInfo){

    if(initialized){
        return;
    }
    initialized = true;

    uint64_t memorySize = GetMemorySize(bootInfo);
    uint64_t totalPages = memorySize / 4096;
    uint64_t bitMapSize = (totalPages / 8) + 1;
    size_t mMapEntries = bootInfo->mMapSize / bootInfo->mMapDescSize;

    reservedMemory = memorySize;
    freeMemory = 0;
    usedMemory = 0;

    void* bitmapLocation = (void*)FindSuitableMemorySegment(bootInfo, bitMapSize);

    InitMemoryBitmap(&bitmap, bitMapSize, (uint8_t*)bitmapLocation);

    for(size_t i = 0; i < mMapEntries; i++){
        MemoryDescriptor* desc = (MemoryDescriptor*)((uint64_t)bootInfo->mMap + (i * bootInfo->mMapDescSize));

        if(desc->type == 7){ 
            UnlockPages((void*)desc->physicalStart, desc->numberOfPages);
        }
    }

    uint64_t pagesForBitmap = (bitmap.size / 4096) + 1;
    void* bitmapStart = (void*)bitmap.address;
    LockPages(bitmapStart, pagesForBitmap);

    uint64_t kernelStart = (uint64_t)&_KernelStart;
    uint64_t kernelEnd = (uint64_t)&_KernelEnd;
    uint64_t kernelSize = kernelEnd - kernelStart;
    uint64_t pagesForKernel = (kernelSize / 4096) + 1;
    LockPages((void*)kernelStart, pagesForKernel);

    LockPages((void*)0, 256);

}

void* RequestPage(){
    uint64_t firstFreeIndex = GetFirstFreeMemoryBit(&bitmap);

    bool result = SetMemoryBit(&bitmap, firstFreeIndex, true);

    if(result){
        uint64_t address = firstFreeIndex * 4096;

        return (void*) address;
    }
}

void FreePage(void* address){

    uint64_t index = ((uint64_t) address) / 4096;

    bool result = SetMemoryBit(&bitmap, index, false);

}

void LockPage(void* address){
    uint64_t index = (uint64_t) address / 4096;
    if(SetMemoryBit(&bitmap, index, true)){
        RESERVE_MEMORY();
    }
}

void LockPages(void* address, uint64_t count){
    for(uint64_t i = 0; i < count; i++){
        void* addr = (void*)((uint64_t)address + (i * 4096));
        LockPage(addr);
    }
}


void UnlockPage(void* address){
    uint64_t index = (uint64_t) address / 4096;
    if(SetMemoryBit(&bitmap, index, false)){
        UNRESERVE_MEMORY();
    }
}

void UnlockPages(void* address, uint64_t count){
    for(uint64_t i = 0; i < count; i++){
        void* addr = (void*)((uint64_t)address + (i * 4096));
        UnlockPage(addr);
    }
}