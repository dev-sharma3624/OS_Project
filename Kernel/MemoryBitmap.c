#include "Typedefs.h"
#include "MemoryBitmap.h"

void InitMemoryBitmap(MemoryBitmap* memoryBitmap, uint64_t size, uint8_t* address){
    memoryBitmap->size = size;
    memoryBitmap->address = address;

    for(uint64_t i = 0; i < size; i++){
        address[i] = 0;
    }
}

bool GetMemoryBit(MemoryBitmap* memoryBitmap, uint64_t index){

    if(index > memoryBitmap->size * 8){
        return false;
    }

    uint8_t byteIndex = index / 8;
    uint8_t bitIndex = index % 8;
    uint8_t bitIndexer = 0b10000000 >> bitIndex;

    if((memoryBitmap->address[byteIndex] & bitIndexer) > 0){
        return true;
    }

    return false;

}

uint64_t GetFirstFreeMemoryBit(MemoryBitmap* memoryBitmap){
    
    uint64_t size = memoryBitmap->size;
    uint8_t bitIndexer = 0b10000000;

    for(uint64_t byteIndex = 0; byteIndex < size; byteIndex++){

        for(uint64_t bitIndex = 0; bitIndex < 8; bitIndex++){

            bool isBitFree = memoryBitmap->address[byteIndex] & (bitIndexer >> bitIndex);

            if(!isBitFree){
                return (byteIndex * 8) + bitIndex;
            }

        }

    }

    return 0;
}

bool SetMemoryBit(MemoryBitmap* memoryBitmap, uint64_t index, bool value){

    if(index > memoryBitmap->size * 8){
        return false;
    }

    uint8_t byteIndex = index / 8;
    uint8_t bitIndex = index % 8;
    uint8_t bitIndexer = 0b10000000 >> bitIndex;

    memoryBitmap->address[byteIndex] &= ~bitIndexer;

    if(value){
        memoryBitmap->address[byteIndex] |= bitIndexer;
    }

    return true;

}