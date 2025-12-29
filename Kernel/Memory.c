#include "Memory.h"
#include "Typedefs.h"

#include "../boot_info.h"
#include "MemoryDescriptor.h"

uint64_t GetMemorySize(BOOT_INFO* bootInfo){

    uint64_t totalMemory = 0;
    
    uint64_t mMapEntries = bootInfo->mMapSize / bootInfo->mMapDescSize;
    uint64_t mMapDescSize = bootInfo->mMapDescSize;

    for(uint64_t i = 0; i < mMapEntries; i++){

        MemoryDescriptor* desc = (MemoryDescriptor*) ((uint64_t)bootInfo->mMap + (i * mMapDescSize));

        if(desc->type == 7){

            uint64_t physicalEnd = desc->physicalStart + (desc->numberOfPages * 4096);

            if(physicalEnd > totalMemory){
                totalMemory = physicalEnd;
            }

        }

    }

    return totalMemory;

}

uint64_t FindSuitableMemorySegment(BOOT_INFO* bootInfo, uint64_t minimumSegmentSize){
    size_t mMapEntries = bootInfo->mMapSize / bootInfo->mMapDescSize;

    for(size_t i = 0; i < mMapEntries; i++){

        MemoryDescriptor* desc = (MemoryDescriptor*) ((uint64_t)bootInfo->mMap + (i * bootInfo->mMapDescSize));

        if(desc->type == 7){

            uint64_t segmentSize = desc->numberOfPages * 4096;
            if(segmentSize > minimumSegmentSize){
                return desc->physicalStart;
            }

        }

    }
}