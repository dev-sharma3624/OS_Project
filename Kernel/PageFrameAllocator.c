#include "Typedefs.h"
#include "../boot_info.h"
#include "Memory.h"
#include "MemoryBitmap.h"
#include "MemoryDescriptor.h"

static MemoryBitmap bitmap;

uint64_t freeMemory;
uint64_t reservedMemory;
uint64_t usedMemory;
bool initialized = false;

void InitPageFrameAllocator(BOOT_INFO* bootInfo){

    if(initialized){
        return;
    }
    initialized = true;

    uint64_t memorySize = GetMemorySize(bootInfo);
    uint64_t totalPages = memorySize / 4096;
    uint64_t bitMapSize = (totalPages / 8) + 1;

    void* bitmapLocation = NULL;

    size_t mMapEntries = bootInfo->mMapSize / bootInfo->mMapDescSize;

    for(size_t i = 0; i < mMapEntries; i++){

        MemoryDescriptor* desc = (MemoryDescriptor*) ((uint64_t)bootInfo->mMap + (i * bootInfo->mMapDescSize));

        if(desc->type == 7){

            uint64_t segmentSize = desc->numberOfPages * 4096;
            if(segmentSize > bitMapSize){
                bitmapLocation = (void*)desc->physicalStart;
                break;
            }

        }

    }

    InitMemoryBitmap(&bitmap, bitMapSize, (uint8_t*)bitmapLocation);

    uint64_t pagesForBitmap = (bitMapSize / 4096) + 1;
    void* bitmapStart = bitmapLocation;
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

    uint8_t index = ((uint64_t) address) / 4096;

    bool result = SetMemoryBit(&bitmap, index, false);

}