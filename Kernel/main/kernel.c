#include "../../boot_info.h"
#include <drivers/font_renderer.h>
#include <libs/k_printf.h>
#include <architecture/x86_64/gdt.h>
#include <architecture/x86_64/idt.h>
#include <architecture/x86_64/pic.h>
#include <drivers/keyboard_driver.h>
// #include <drivers/keyboard_map.h>
#include <memory_management/memory.h>
#include <typedefs.h>
#include <memory_management/pmm.h>
#include <memory_management/paging.h>
#include <memory_management/heap.h>
#include <drivers/timer.h>
#include <cpu_scheduling/process.h>
#include <cpu_scheduling/scheduler.h>
#include <memory_management/m_desc.h>
#include <drivers/pci.h>

#define MAX_COMMAND_BUFFER 256
char command_buffer[MAX_COMMAND_BUFFER];
int buffer_position = 0;

boot_info_t boot_info;

int kernel_str_cmp(const char* str_1, const char* str_2){

    while(*str_1 && (*str_1 == *str_2)){
        str_1++;
        str_2++;
    }

    return *(const unsigned char*) str_1 - *(const unsigned char*)str_2;

}

void kernel_clear_buffer(){
    for(int i = 0; i < MAX_COMMAND_BUFFER; i++){
        command_buffer[i] = 0;
    }
    buffer_position = 0;
}


void kernel_print_memory_info() {
    uint64_t total_memory = free_memory + reserved_memory + used_memory;

    k_printf("\n--- Memory Statistics ---\n");
    k_printf("Total RAM:    %d MB\n", total_memory / 1024 / 1024);
    k_printf("Free RAM:     %d MB\n", free_memory / 1024 / 1024);
    k_printf("Used RAM:     %d KB\n", used_memory / 1024);
    k_printf("Reserved RAM: %d MB\n", reserved_memory / 1024 / 1024);

    k_printf("-------------------------\n");
}

void sys_read(char* buffer) {
    asm volatile (
        "mov $1, %%rax\n"
        "mov %0, %%rdi\n"
        "int $0x80\n"
        :
        : "r"(buffer)
        : "rax", "rdi"
    );
}

void sys_print(char* msg){
    asm volatile (
        "mov $0, %%rax\n"
        "mov %0, %%rbx\n"
        "int $0x80\n"
        :
        : "r"(msg)
        : "rax", "rbx"
    );
}

void sys_exit(int code) {
    asm volatile (
        "mov $2, %%rax\n"
        "mov %0, %%rdi\n"
        "int $0x80\n"
        :
        : "r"((uint64_t)code)
        : "rax", "rdi"
    );
    while(1) {} 
}

void* sys_sbrk(int64_t increment) {
    void* ret;
    asm volatile (
        "mov $3, %%rax\n"
        "mov %1, %%rdi\n"
        "int $0x80\n"
        "mov %%rax, %0\n"
        : "=r"(ret)
        : "r"(increment)
        : "rax", "rdi"
    );
    return ret;
}

void* malloc(uint64_t size) {
    if (size == 0) return 0;
    
    void* ptr = sys_sbrk(size);
    
    // Check if sbrk failed (returned -1)
    if (ptr == (void*)-1) {
        return 0;
    }
    
    return ptr;
}

void background_counter() {
    int count = 0;
    while(1) {
        // Busy wait (approx 1 sec)
        for(volatile int i=0; i<50000000; i++); 
        
        // Print safely
        // Note: Make sure sys_print uses your lock!
        sys_print(" [BG] "); 
        
        count++;
    }
}

void test_user_function() {
    /* sys_print("Hello! I am going to count to 3 and then vanish.\n");
    
    sys_print("1...\n");
    sys_print("2...\n");
    sys_print("3...\n");
    
    sys_print("Goodbye cruel world!\n");
    
    sys_exit(0);
    
    sys_print("ERROR: I am a ghost!\n"); */

    char input_char;
    char echo_buf[2];
    echo_buf[1] = 0; // Null terminator

    sys_print("Interactive Shell v1.0\n");
    sys_print("> ");

    while (1) {
        // This will BLOCK the process until you press a key!
        sys_read(&input_char); 

        // Echo it back
        echo_buf[0] = input_char;
        sys_print(echo_buf);
        
        if (input_char == '\n') {
             sys_print("> ");
        }
    }
}

// 2. The Switch Function
void jump_to_user_mode() {
    
    // A. Define the Ring 3 Selectors based on your GDT
    uint64_t user_cs = 0x23; // Index 4 | RPL 3
    uint64_t user_ds = 0x2B; // Index 5 | RPL 3
    uint64_t rflags  = 0x202; // Interrupts ENABLED (Bit 9) + Reserved (Bit 1)

    // B. Setup a temporary stack for the user process
    // (In a real OS, you'd malloc this. For now, a small static array works)
    static uint8_t user_stack[4096];
    uint64_t user_rsp = (uint64_t)user_stack + 4096;

    // C. The Target Address
    uint64_t entry_point = (uint64_t)&test_user_function;

    // D. The "IRETQ" Frame Setup (Inline Assembly)
    // We push the 5 values in this EXACT order:
    // SS -> RSP -> RFLAGS -> CS -> RIP
    asm volatile (
        // FIX: Use %w0 to grab the lower 16-bits (Word) of the 64-bit register
        "mov %w0, %%ax\n"  
        "mov %%ax, %%ds\n"
        "mov %%ax, %%es\n"
        "mov %%ax, %%fs\n"
        "mov %%ax, %%gs\n"
        
        // Stack Setup for IRETQ (Must be 64-bit pushes)
        "pushq %0\n"       // 1. SS (We push the full 64-bit val, CPU uses low 16)
        "pushq %1\n"       // 2. RSP
        "pushq %2\n"       // 3. RFLAGS
        "pushq %3\n"       // 4. CS
        "pushq %4\n"       // 5. RIP
        
        "iretq\n"
        : 
        : "r"(user_ds), "r"(user_rsp), "r"(rflags), "r"(user_cs), "r"(entry_point)
        : "rax", "memory"
    );
}


void kernel_execute_command(){
    k_printf("\n");

    if(kernel_str_cmp(command_buffer, "help") == 0){
        k_printf("Available commands:\n");
        k_printf(" - help: Show this menu\n");
        k_printf(" - clear: Clean the screen\n");
        k_printf(" - reboot: Reboot the CPU\n");
        k_printf(" - meminfo: See how much RAM is being used\n");
        k_printf(" - drums of liberation: Awaken the Sun God!\n");
        k_printf(" - jump\n");
    }

    else if(kernel_str_cmp(command_buffer, "clear") == 0){
        font_renderer_clear_screen();
    }

    else if(kernel_str_cmp(command_buffer, "meminfo") == 0){
        kernel_print_memory_info();
    }

    else if(kernel_str_cmp(command_buffer, "drums of liberation") == 0){
        k_printf("THE ONE PIECE IS REAL!\n");
    }

    else if(kernel_str_cmp(command_buffer, "jump") == 0){
        jump_to_user_mode();
    }

    else if (buffer_position > 0){
        k_printf("Unknown command: ");
        k_printf("%s\n", command_buffer);
    };

    kernel_clear_buffer();
    k_printf("Project D> ");
    
}

void task_A() {
    while(1) {
        for(volatile int i = 0; i < 10000000; i++);
        for(volatile int j = 0; j < 10000000; j++);
        for(volatile int k = 0; k < 10000000; k++);
    }
}

void task_B() {
    while(1) {
        k_printf(" [B] ");
        task_sleep(500);
    }
}

void sanitize_boot_info(boot_info_t* boot_info_recieved){

    //elements that don't need physical->virtual conversion 
    boot_info.m_map_desc_size = boot_info_recieved->m_map_desc_size;
    boot_info.m_map_size = boot_info_recieved->m_map_size;

    //physical->virtual converision elements
    boot_info_recieved->font->header = (psf1_header_t*) P2V(boot_info_recieved->font->header);
    boot_info_recieved->font->glyph_buffer = (void*) P2V(boot_info_recieved->font->glyph_buffer);
    boot_info.m_map = (void*) P2V(boot_info_recieved->m_map);
    boot_info.font = (psf1_font_t*) P2V(boot_info_recieved->font);

    //special case, since we need to pass this as physical address for mapping
    boot_info.frame_buffer = boot_info_recieved->frame_buffer;
}

void initialize_desc_tables(){
    gdt_init();
    idt_init();
}

void initialize_drivers(){
    remap_pic();
    timer_init(1000);
}

void setup_memory_management(){

    pmm_init(&boot_info);

    //code to test pmm page allocation, freeing and recycling
    // void* page1 = pmm_request_page();
    // void* page2 = pmm_request_page();
    // void* page3 = pmm_request_page();
    // void* page4= pmm_request_page();
    // pmm_free_page(page1);
    // void* page5 = pmm_request_page();
    // if(page1 == page5){
    //     io_print("PMM RECYCLE\n");
    // }

    uint64_t m_map_size = memory_get_m_size(&boot_info);

    paging_init(
        boot_info.frame_buffer.frame_buffer_base, 
        boot_info.frame_buffer.frame_buffer_size,
        m_map_size
    );

    void* heap_start = pmm_request_page();
    for(int i = 0; i < 9; i++) pmm_request_page();

    uint64_t heap_start_virt_addr = P2V(heap_start);
    heap_init((void*)heap_start_virt_addr, 4096 * 10);

    // code to test heap allocation and de-allocation
    // void* ptr_a = heap_kmalloc(10);
    // void* ptr_b = heap_kmalloc(20);

    // k_printf("Ptr A: %x\n", (uint64_t)ptr_a);
    // k_printf("Ptr B: %x\n", (uint64_t)ptr_b);

    // k_printf("Freeing Ptr A...\n");
    // heap_kfree(ptr_a);

    // void* ptr_c = heap_kmalloc(5);
    // k_printf("Ptr C: %x\n", (uint64_t)ptr_c);

    // if (ptr_c == ptr_a) {
    //     k_printf("SUCCESS: Heap reused the freed memory!\n");
    // } else {
    //     k_printf("FAIL: Heap created a new block instead of reusing.\n");
    // }

}

void initialize_frame_renderer(){
    //converting physical->virtual since frame buffer was left to be sanitized after page table setup
    boot_info.frame_buffer.frame_buffer_base = (void*)P2V(boot_info.frame_buffer.frame_buffer_base);

    uint32_t text_color = 0xFF000000;
    uint32_t bg_color = 0xFFFF8000;

    font_renderer_init(&boot_info.frame_buffer, boot_info.font, text_color, bg_color);

    font_renderer_clear_screen();
}

void initialize_multitasking(){
    multitask_init();
}

void setup_nvme(){

    k_printf("Scanning PCI Bus...\n");
    
    pci_device_info_t nvme = pci_scan_for_device(CLASS_MASS_STORAGE, SUBCLASS_NVME);
    
    if (nvme.found) {
        k_printf("SUCCESS: NVMe Drive Found!\n");
        k_printf(" - Bus: %d, Slot: %d\n", nvme.bus, nvme.slot);
        k_printf(" - Physical BAR Address: %p\n", nvme.physical_address);
    } else {
        k_printf("FAILURE: No NVMe Drive found. Check QEMU flags!\n");
    }

    uint64_t nvme_virt_addr = P2V(nvme.physical_address);

    for(uint64_t i = 0; i < 4; i++){
        paging_map_page(
            get_kernel_page_table(),
            (void*) (nvme_virt_addr + i*4096),
            nvme.physical_address + (i*4096),
            PT_FLAG_PRESENT | PT_FLAG_READ_WRITE | PT_FLAG_WRITE_THROUGH | PT_FLAG_CACHE_DISABLED  
        );
    }

    //flush the TLB for this address so that cpu forgets any old mappings
    __asm__ volatile("invlpg (%0)" :: "r" (nvme_virt_addr));

    volatile uint64_t* nvme_regs = (volatile uint64_t*)nvme_virt_addr;

    // offset 0 : Capabilities regiester
    uint64_t cap = nvme_regs[0];

    // Bits 0-15: Max Queue Entries Supported (MQES) - The value is 0-based
    uint16_t max_queue_entries = (cap & 0xFFFF) + 1;
    
    // Bits 24-31: Timeout (TO) in 500ms units
    uint8_t timeout = (cap >> 24) & 0xFF; 

    // Bits 32-35: Doorbell Stride (DSTRD) - How far apart the doorbells are spaced
    uint8_t doorbell_stride = (cap >> 32) & 0xF;

    k_printf("[NVMe] CAP Register Read: %p\n", cap);
    k_printf("Max Queue Size: %d", max_queue_entries);
    k_printf("Timeout limit: %d\n", timeout);;
    k_printf("Doorbell Stride: 2^(%d) bytes\n", 2+doorbell_stride);

    if (cap == 0xFFFFFFFFFFFFFFFF) {
        k_printf("[ERROR] Read -1. Mapping Failed or Device Gone.\n");
    } else if (cap == 0) {
        k_printf("[ERROR] Read 0. Mapping likely pointing to empty RAM.\n");
    } else {
        k_printf("[SUCCESS] We are talking to the NVMe Controller!\n");
    }
}

void kernel_start(boot_info_t* boot_info_recieved){

    if(!boot_info_recieved) return;

    // Initialize serial port logging, uncomment if needed again
    // io_init();
    // io_print("\n[KERNEL] Higher Half Kernel Started!\n");
    // print_address_hex((void*)boot_info_recieved);
    // print_dec(99999);

    //enable interrupts
    __asm__ volatile ("sti");

    sanitize_boot_info(boot_info_recieved);
    initialize_desc_tables();
    initialize_drivers();
    setup_memory_management();
    initialize_frame_renderer();
    initialize_multitasking();

    // create_task(&task_A);

    setup_nvme();

    while(1);

    /* k_printf("Project D v0.1. Type 'help'.\nProject D> ");
    
    while (1)
    {
        uint8_t scan_code = read_key();
        if(scan_code != 0){

            if(scan_code & 0x80){ //release key code
                continue;
            }
            
            if(scan_code == 0x0E){ //backspace key

                if(buffer_position > 0){

                    buffer_position--;
                    command_buffer[buffer_position] = 0;

                    k_printf("\b");
                }
            }

            if (scan_code == 0x1C){ //enter key
                kernel_execute_command();
                continue;
            }

            if(scan_code < 0x3A){ //lowercase alphabet keys (not including any special characters, numbers or uppercase)
                char ascii = scan_code_for_lookup_table[scan_code];

                if(ascii != 0){

                    if(buffer_position < MAX_COMMAND_BUFFER - 1){

                        k_printf("%c", ascii);
                        command_buffer[buffer_position] = ascii;
                        buffer_position++;

                    }
                }
            }
        }

    } */
}

    /* 

    ................................................................................................
    ................................................................................................

    Here, we successfully implemented a virtual memory manager, initialized it and loaded the pml4
    table to cr3 register.

    ................................................................................................
    ................................................................................................ 


    font_renderer_init(&boot_info.frame_buffer, boot_info.font, 0xff000000, 0xFFFF8000);

    font_renderer_clear_screen();

    k_printf("Kernel initialized.\n");
    k_printf("Loading GDT...\n");

    gdt_init();

    k_printf("GDT loaded successfully\n");

    k_printf("Loading IDT...\n");

    idt_init();
    k_printf("IDT loaded successfully\n");

    k_printf("Remapping PIC...\n");
    remap_pic();
    k_printf("PIC remapping successfull\n");

    uint64_t m_map_size = memory_get_m_size(&boot_info);

    k_printf("Total memory: %p bytes\n", m_map_size);
    k_printf("Total memory: %d MB\n", (int) m_map_size / 1024 / 1024);

    pmm_init(&boot_info);
    k_printf("Page Frame Allocator Initialized!\n");

    void* page1 = pmm_request_page();
    k_printf("Page 1 request: %p\n", (uint64_t)page1);

    void* page2 = pmm_request_page();
    k_printf("Page 2 request: %p\n", (uint64_t)page2);

    void* page3 = pmm_request_page();
    k_printf("Page 3 Request: %p\n", (uint64_t)page3);

    void* page4= pmm_request_page();
    k_printf("Page 4 Request: %p\n", (uint64_t)page4);
    
    k_printf("Freeing Page 1...\n");
    pmm_free_page(page1);

    void* page5 = pmm_request_page();
    k_printf("Page 5 Request: %p\n", (uint64_t)page5);


    if (page5 == page1) {
        k_printf("TEST PASSED: Memory Recycled!\n");
    } else {
        k_printf("TEST FAILED: Allocator ignored the free page.\n");
    }

    paging_init(
        boot_info.frame_buffer.frame_buffer_base, 
        boot_info.frame_buffer.frame_buffer_size,
        m_map_size
    );
    k_printf("VMM Initialized!\n");

    __asm__ volatile ("sti"); 

    k_printf("Input enabled.\n\n");

    k_printf("Project D v0.1. Type 'help'.\nProject D> ");
    
    while (1)
    {
        unsigned char scan_code = read_key();
        if(scan_code != 0){

            if(scan_code & 0x80){
                continue;
            }
            
            if(scan_code == 0x0E){

                if(buffer_position > 0){

                    buffer_position--;
                    command_buffer[buffer_position] = 0;


                    k_printf("\b");
                }
            }

            else if (scan_code == 0x1C){
                kernel_execute_command();
            }

            if(scan_code < 0x3A){
                char ascii = scan_code_for_lookup_table[scan_code];

                if(ascii != 0){

                    if(buffer_position < MAX_COMMAND_BUFFER - 1){

                        k_printf("%c", ascii);
                        command_buffer[buffer_position] = ascii;
                        buffer_position++;

                    }
                }
            }
        }

        __asm__ volatile ("hlt");

    }*/


    /* 

    ................................................................................................
    ................................................................................................

    Here, we successfully implemented a physical memory manager using bitmap to allocate, free and
    track phyiscal memory pages.

    ................................................................................................
    ................................................................................................ 
    font_renderer_init(&boot_info.frame_buffer, boot_info.font, 0xff000000, 0xFFFF8000);

    font_renderer_clear_screen();

    k_printf("Kernel initialized.\n");
    k_printf("Loading GDT...\n");

    gdt_init();

    k_printf("GDT loaded successfully\n");

    k_printf("Loading IDT...\n");

    idt_init();
    k_printf("IDT loaded successfully\n");

    k_printf("Remapping PIC...\n");
    remap_pic();
    k_printf("PIC remapping successfull\n");

    uint64_t m_map_size = memory_get_m_size(&boot_info);

    k_printf("Total memory: %p bytes\n", m_map_size);
    k_printf("Total memory: %d MB\n", (int) m_map_size / 1024 / 1024);

    pmm_init(&boot_info);
    k_printf("Page Frame Allocator Initialized!\n");

    void* page1 = pmm_request_page();
    k_printf("Page 1 request: %p\n", (uint64_t)page1);

    void* page2 = pmm_request_page();
    k_printf("Page 2 request: %p\n", (uint64_t)page2);

    void* page3 = pmm_request_page();
    k_printf("Page 3 Request: %p\n", (uint64_t)page3);

    void* page4= pmm_request_page();
    k_printf("Page 4 Request: %p\n", (uint64_t)page4);
    
    k_printf("Freeing Page 1...\n");
    pmm_free_page(page1);

    void* page5 = pmm_request_page();
    k_printf("Page 5 Request: %p\n", (uint64_t)page5);


    if (page5 == page1) {
        k_printf("TEST PASSED: Memory Recycled!\n");
    } else {
        k_printf("TEST FAILED: Allocator ignored the free page.\n");
    }

    __asm__ volatile ("sti"); 

    k_printf("Input enabled.\n\n");

    k_printf("Project D v0.1. Type 'help'.\nProject D> ");
    
    while (1)
    {
        unsigned char scan_code = read_key();
        if(scan_code != 0){

            if(scan_code & 0x80){
                continue;
            }
            
            if(scan_code == 0x0E){

                if(buffer_position > 0){

                    buffer_position--;
                    command_buffer[buffer_position] = 0;


                    k_printf("\b");
                }
            }

            else if (scan_code == 0x1C){
                kernel_execute_command();
            }

            if(scan_code < 0x3A){
                char ascii = scan_code_for_lookup_table[scan_code];

                if(ascii != 0){

                    if(buffer_position < MAX_COMMAND_BUFFER - 1){

                        k_printf("%c", ascii);
                        command_buffer[buffer_position] = ascii;
                        buffer_position++;

                    }
                }
            }
        }

        __asm__ volatile ("hlt");

    }
     */

    

    /*

    ................................................................................................
    ................................................................................................

    Here, we successfully used the memory data passed from the bootloader to calculate the total
    available memory(RAM).

    ................................................................................................
    ................................................................................................ 

    font_renderer_init(&boot_info.frame_buffer, boot_info.font, 0xff000000, 0xFFFF8000);

    font_renderer_clear_screen();

    k_printf("Kernel initialized.\n");
    k_printf("Loading GDT...\n");

    gdt_init();

    k_printf("GDT loaded successfully\n");

    k_printf("Loading IDT...\n");

    idt_init();
    k_printf("IDT loaded successfully\n");

    k_printf("Remapping PIC...\n");
    remap_pic();
    k_printf("PIC remapping successfull\n");

    uint64_t m_map_size = memory_get_m_size(&boot_info);

    k_printf("Total memory: %p bytes\n", m_map_size);
    k_printf("Total memory: %d MB\n", (int) m_map_size / 1024 / 1024);

    __asm__ volatile ("sti"); 

    k_printf("Input enabled.\n\n");

    k_printf("Project D v0.1. Type 'help'.\nProject D> ");
    
    while (1)
    {
        unsigned char scan_code = read_key();
        if(scan_code != 0){

            if(scan_code & 0x80){
                continue;
            }
            
            if(scan_code == 0x0E){

                if(buffer_position > 0){

                    buffer_position--;
                    command_buffer[buffer_position] = 0;


                    k_printf("\b");
                }
            }

            else if (scan_code == 0x1C){
                kernel_execute_command();
            }

            if(scan_code < 0x3A){
                char ascii = scan_code_for_lookup_table[scan_code];

                if(ascii != 0){

                    if(buffer_position < MAX_COMMAND_BUFFER - 1){

                        k_printf("%c", ascii);
                        command_buffer[buffer_position] = ascii;
                        buffer_position++;

                    }
                }
            }
        }

        __asm__ volatile ("hlt");

    } */

    /* 

    ................................................................................................
    ................................................................................................

    This version of kernel had a basic working shell with few commands working!

    ................................................................................................
    ................................................................................................
    
    font_renderer_init(&boot_info.frame_buffer, boot_info.font, 0xff000000, 0xFFFF8000);

    font_renderer_clear_screen();

    k_printf("Kernel initialized.\n");
    k_printf("Loading GDT...\n");

    gdt_init();

    k_printf("GDT loaded successfully\n");

    k_printf("Loading IDT...\n");

    idt_init();
    k_printf("IDT loaded successfully\n");

    k_printf("Remapping PIC...\n");
    remap_pic();
    k_printf("PIC remapping successfull\n");

    __asm__ volatile ("sti"); 

    k_printf("Input enabled.\n\n\n\n");

    k_printf("DevOS v0.1. Type 'help'.\nDevOS> ");
    
    while (1)
    {
        unsigned char scan_code = read_key();
        if(scan_code != 0){

            if(scan_code & 0x80){
                continue;
            }
            
            if(scan_code == 0x0E){

                if(buffer_position > 0){

                    buffer_position--;
                    command_buffer[buffer_position] = 0;


                    k_printf("\b");
                }
            }

            else if (scan_code == 0x1C){
                kernel_execute_command();
            }

            if(scan_code < 0x3A){
                char ascii = scan_code_for_lookup_table[scan_code];

                if(ascii != 0){

                    if(buffer_position < MAX_COMMAND_BUFFER - 1){

                        k_printf("%c", ascii);
                        command_buffer[buffer_position] = ascii;
                        buffer_position++;

                    }
                }
            }
        }

        __asm__ volatile ("hlt");

    } */

    /* 

    ................................................................................................
    ................................................................................................

    This version of kernel was successfully able to print text on screen from keyboard interrupts.
    Keyboard became alive here! 

    ................................................................................................
    ................................................................................................
    
    font_renderer_init(&boot_info.frame_buffer, boot_info.font, 0xff000000, 0xFFFF8000);

    font_renderer_clear_screen();

    k_printf("Kernel initialized.\n");
    k_printf("Loading GDT...\n");

    gdt_init();

    k_printf("GDT loaded successfully\n");

    k_printf("Loading IDT...\n");

    idt_init();
    k_printf("IDT loaded successfully\n");

    k_printf("Remapping PIC...\n");
    remap_pic();
    k_printf("PIC remapping successfull\n");

    __asm__ volatile ("sti"); 

    k_printf("Input enabled.\n\n");
    
    while (1)
    {
        unsigned char scan_code = read_key();
        if(scan_code != 0){
            if(scan_code < 0x3A){
                char ascii = scan_code_for_lookup_table[scan_code];

                if(ascii != 0){
                    k_printf("%c", ascii);
                }
            }
        }

    } */

    /* 

    ................................................................................................
    ................................................................................................

    This version of kernel successfully remapped PIC to move hardware interrupts from 0-15 to 32-47

    ................................................................................................
    ................................................................................................
    
    font_renderer_init(&boot_info.frame_buffer, boot_info.font, 0xff000000, 0xFFFF8000);

    font_renderer_clear_screen();

    k_printf("Kernel initialized.\n");
    k_printf("Loading GDT...\n");

    gdt_init();

    k_printf("GDT loaded successfully\n");

    k_printf("Loading IDT...\n");

    idt_init();
    k_printf("IDT loaded successfully\n");

    k_printf("Remapping PIC...\n");
    remap_pic();
    k_printf("PIC remapping successfull\n");
    while (1)
    {
        __asm__ ("hlt");
    } */


    /* 

    ................................................................................................
    ................................................................................................

    This version of kernel was loading the GDT and IDT and testing a "Divide by zero" error that
    triggers vector 0 of the interrupt table.

    ................................................................................................
    ................................................................................................

    
    font_renderer_init(&boot_info.frame_buffer, boot_info.font, 0xff000000, 0xFFFF8000);

    font_renderer_clear_screen();

    k_printf("Kernel initialized.\n");
    k_printf("Loading GDT...\n");

    gdt_init();

    k_printf("GDT loaded successfully\n");

    k_printf("Loading IDT...\n");

    idt_init();
    k_printf("IDT loaded successfully\n");

    k_printf("Preparing to crash...\n");

    volatile int a = 10;
    volatile int b = 0;
    int c = a / b;

    k_printf("If you see this, the OS survived (which is bad!)\n"); 


    ***********************************************************************************************
    ***********************************************************************************************

    This is the dummy handler function that was getting triggered on divide by zero exception in this
    version of kernel. Keeping it here in case it gets deleted from Idt.c file in later versions.

    void idt_dummy_handler() {
        k_printf("\n");
        k_printf("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
        k_printf("!!!      INTERRUPT RECEIVED          !!!\n");
        k_printf("!!!   Vector 0: Divide By Zero       !!!\n");
        k_printf("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
        
        // Halt the CPU forever so we can read the message
        while(1) {
            __asm__ volatile ("hlt");
        }
    }


    ***********************************************************************************************
    ***********************************************************************************************
    
    */

    /* 

    ................................................................................................
    ................................................................................................

    This version of kernel was loading the Global Descriptor Table to the CPU registers and verifying
    the code segment register value to verfiy whether our GDT was loaded successfully or not.

    ................................................................................................
    ................................................................................................

    font_renderer_init(&boot_info.frame_buffer, boot_info.font, 0xff000000, 0xFFFF8000);
    
    font_renderer_clear_screen();

    k_printf("Kernel initialized.\n");
    k_printf("Loading GDT...\n");

    gdt_init();

    unsigned short int currentCS;

    __asm__ volatile ("mov %%cs, %0" : "=r"(currentCS));

    k_printf("Current CS Register: %x\n", currentCS);

    k_printf("GDT loaded successfully"); */
    

    /*

    ................................................................................................
    ................................................................................................

    This version of kernel was meant to test the k_printf() method that will help us print variables
    like decimals, hexadecimals etc to the screen thus helping us in printing debugging messages.

    ................................................................................................
    ................................................................................................

    frame_buffer_t frame_buffer = boot_info.frame_buffer;
    psf1_font_t* font = boot_info.font;

    // 1. Initialize the Global Renderer
    font_renderer_init(&frame_buffer, font, 0xff000000, 0xFFFF8000);

    // 2. Clear the screen (Blue background usually, or whatever your default is)
    font_renderer_clear_screen();

    // 3. Test 1: Basic String & Char
    k_printf("Test 1: Hello World! %c\n", 'A');

    // 4. Test 2: Decimal Integers (Positive & Negative)
    k_printf("Test 2: Integer: %d, Negative: %d\n", 1234, -5678);

    // 5. Test 3: Hexadecimal
    // 255 should print as 0xFF (or 0xff depending on your implementation)
    k_printf("Test 3: Hex: %x\n", 255);

    // 6. Test 4: Pointers
    // This prints the memory address of the boot_info struct
    k_printf("Test 4: Pointer Address: %p\n", boot_info);

    // 7. Test 5: Mixed Formatting
    k_printf("Test 5: Mixed: %s is %d years old.\n", "Dev", 21); */


    /* 

    ................................................................................................
    ................................................................................................

    This version of kernel was meant to test the basic_renderer_t methods that would facilitate printing
    of strings on the screen using the psf1 font.

    ................................................................................................
    ................................................................................................
    
    font_renderer_init(&frame_buffer, font, 0xff000000, 0xFFFF8000);
    
    font_renderer_clear_screen();
    font_renderer_print("Print is working\n");
    font_renderer_print("Font renderer is working!"); */

    /*

    ................................................................................................
    ................................................................................................

    This version of kernel was meant to change the color of entire screen by directly manipulating
    the frame_buffer data by iterating from frame_buffer_base to framebuffer size.

    ................................................................................................
    ................................................................................................
    
     unsigned int* frame_buffer_base = (unsigned int*)boot_info->frame_buffer.frame_buffer_base;
    unsigned int frame_buffer_size = boot_info->frame_buffer.frame_buffer_size;

    unsigned int color = 0xFF0000FF;

    for(unsigned int i = 0; i < frame_buffer_size / 4; i++){
        frame_buffer_base[i] = color;
    }
 */