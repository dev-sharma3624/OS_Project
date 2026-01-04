#include <typedefs.h>
#include <memory_management/heap.h>

heap_segment_header_t* first_segment = NULL;

void heap_init(void* heap_start, size_t heap_size){

    first_segment = (heap_segment_header_t*) heap_start;

    first_segment->length = heap_size - sizeof(heap_segment_header_t);

    first_segment->last = NULL;
    first_segment->next = NULL;
    first_segment->is_free = true;

}

void* heap_kmalloc(size_t size){

    if(size % 8 != 0){
        size += 8 - (size % 8);
    }

    heap_segment_header_t* current_segment = first_segment;

    while (true) {

        if(current_segment->is_free){

            if(current_segment->length > sizeof(heap_segment_header_t) + size){

                size_t current_address = (size_t) current_segment;
                size_t offset = sizeof(heap_segment_header_t) + size;

                heap_segment_header_t* remaining_free_segment = (heap_segment_header_t*) (current_address + offset);

                remaining_free_segment->next = current_segment->next;
                remaining_free_segment->last = current_segment;
                remaining_free_segment->is_free = true;
                remaining_free_segment->length = current_segment->length - (sizeof(heap_segment_header_t) + size);

                current_segment->next = remaining_free_segment;
                current_segment->length = size;
                current_segment->is_free = false;

                return (void*) (current_address + sizeof(heap_segment_header_t));
                
            }

            if(current_segment->length >= size){
                current_segment->is_free = false;
                return (void*) ((size_t)current_segment + sizeof(heap_segment_header_t));
            }

        }

        if(current_segment->next == NULL){
            break;
        }

        current_segment = current_segment->next;

    }

    return NULL;

}

void heap_merge(heap_segment_header_t* current_segment){

    if(current_segment->next == NULL){
        return;
    }

    if(current_segment->next->is_free){
        current_segment->length += current_segment->next->length + sizeof(heap_segment_header_t);
        current_segment->next = current_segment->next->next;

        if(current_segment->next != NULL){
            current_segment->next->last = current_segment;
        }
    }

}


void heap_kfree(void* address){

    heap_segment_header_t* current_segment = (heap_segment_header_t*) (address - sizeof(heap_segment_header_t));

    current_segment->is_free = true;

    heap_merge(current_segment);

    if(current_segment->last != NULL && current_segment->last->is_free){
        heap_merge(current_segment->last);
    }

}