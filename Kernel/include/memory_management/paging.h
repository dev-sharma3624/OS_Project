#include <typedefs.h>

typedef enum {
    PT_FLAG_PRESENT = 1 << 0,
    PT_FLAG_READ_WRITE = 1 << 1,
    PT_FLAG_USER_SUPER = 1 << 2,
    PT_FLAG_WRITE_THROUGH = 1 << 3,
    PT_FLAG_CACHE_DISABLED = 1 << 4,
    PT_FLAG_ACCESSED = 1 << 5,
    PT_FLAG_DIRTY = 1 << 6,
    PT_FLAG_HUGE_PAGE = 1 << 7,
    PT_FLAG_GLOBAL = 1 << 8,
    PT_FLAG_NX = 1UL << 63,
} paging_flags_t;

typedef struct {
    uint64_t present : 1;
    uint64_t read_write : 1;
    uint64_t user_super : 1;
    uint64_t write_through : 1;
    uint64_t cache_disabled : 1;
    uint64_t accessed : 1;
    uint64_t dirty : 1;
    uint64_t huge_page : 1;
    uint64_t global : 1;
    uint64_t available : 3;
    uint64_t address : 40;
    uint64_t reserved : 11;
    uint64_t nx : 1;
} __attribute__((packed)) paging_map_entry_t;

typedef struct {
    paging_map_entry_t entries[512];
} __attribute__((aligned(4096))) paging_page_table_t;

void paging_map_page(paging_page_table_t* pml4, void* virtual_addr, void* physical_addr, uint64_t flags);
void paging_init(void* frame_buffer_addr, uint64_t frame_buffer_size, uint64_t total_ram_size);

