#include <typedefs.h>
#include <memory_management/heap.h>
#include <memory_management/pmm.h>
#include <memory_management/paging.h>

#define PAGE_SIZE 4096
#define HEADER_SIZE() sizeof(heap_segment_header_t)

heap_segment_header_t* first_segment = NULL; //starting node of the list that tracks heap memory
void* heap_end_address = NULL; //the virtual address where the heap tracking list ends
size_t total_heap_size;

size_t get_heap_size(){
    return total_heap_size;
}

void heap_init(void* heap_start, size_t heap_size){

    first_segment = (heap_segment_header_t*) heap_start;

    first_segment->length = heap_size - sizeof(heap_segment_header_t);

    first_segment->last = NULL;
    first_segment->next = NULL;
    first_segment->is_free = true;

    heap_end_address = (void*) ((size_t)heap_start + heap_size);
    total_heap_size = heap_size;

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

void heap_expand(size_t expansion_size){
    size_t total_size = expansion_size + HEADER_SIZE();

    if(total_size % PAGE_SIZE == 0){
        total_size += PAGE_SIZE - (total_size % PAGE_SIZE);
    }

    size_t total_pages = total_size / PAGE_SIZE;

    void* start_of_new_chunk = heap_end_address;

    for(size_t i = 0; i < total_pages; i++){
        void* phys_page = pmm_request_page();

        paging_map_page(
            get_kernel_page_table(),
            (void*) ((size_t)heap_end_address + (i * PAGE_SIZE)),
            (size_t) phys_page,
            PT_FLAG_PRESENT | PT_FLAG_READ_WRITE | PT_FLAG_USER_SUPER
        );

    }

    heap_end_address = (void*) ((size_t)heap_end_address + total_size);

    heap_segment_header_t* new_segment = (heap_segment_header_t*) start_of_new_chunk;
    new_segment->length = total_size - HEADER_SIZE();
    new_segment->is_free = true;
    new_segment->next = NULL;

    heap_segment_header_t* segment_list = first_segment;
    while(segment_list->next != NULL){
        segment_list = segment_list->next;
    }

    new_segment->last = segment_list;
    segment_list->next = new_segment;

    total_heap_size += total_size;

    heap_merge(new_segment->last);

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
            heap_expand(size);
        }else{
            current_segment = current_segment->next;
        }

    }

    return NULL;

}


void heap_kfree(void* address){

    heap_segment_header_t* current_segment = (heap_segment_header_t*) (address - sizeof(heap_segment_header_t));

    current_segment->is_free = true;

    heap_merge(current_segment);

    if(current_segment->last != NULL && current_segment->last->is_free){
        heap_merge(current_segment->last);
    }

}