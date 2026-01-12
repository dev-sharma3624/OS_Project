#include <typedefs.h>
#include <memory_management/m_bitmap.h>

void m_bitmap_init(memory_bitmap* memory_bitmap, uint64_t size, uint8_t* address){
    memory_bitmap->size = size;
    memory_bitmap->address = address;

    for(uint64_t i = 0; i < size; i++){
        address[i] = 0xFF;
    }
}

bool m_bitmap_get_memory_bit(memory_bitmap* memory_bitmap, uint64_t index){

    if(index > memory_bitmap->size * 8){
        return false;
    }

    uint64_t byte_index = index / 8;
    uint8_t bit_index = index % 8;
    uint8_t bit_indexer = 0b10000000 >> bit_index;

    if((memory_bitmap->address[byte_index] & bit_indexer) > 0){
        return true;
    }

    return false;

}

uint64_t m_bitmap_get_first_free_memory_bit(memory_bitmap* memory_bitmap){
    
    uint64_t size = memory_bitmap->size;
    uint8_t bit_indexer = 0b10000000;

    for(uint64_t byte_index = 0; byte_index < size; byte_index++){

        for(uint64_t bit_index = 0; bit_index < 8; bit_index++){

            bool is_bit_free = memory_bitmap->address[byte_index] & (bit_indexer >> bit_index);

            if(!is_bit_free){
                return (byte_index * 8) + bit_index;
            }

        }

    }

    return 0;
}

bool m_bitmap_get_set_memory_bit(memory_bitmap* memory_bitmap, uint64_t index, bool value){

    if(index > memory_bitmap->size * 8){
        return false;
    }

    uint64_t byte_index = index / 8;
    uint64_t bit_index = index % 8;
    uint64_t bit_indexer = 0b10000000 >> bit_index;

    memory_bitmap->address[byte_index] &= ~bit_indexer;

    if(value){
        memory_bitmap->address[byte_index] |= bit_indexer;
    }

    return true;

}