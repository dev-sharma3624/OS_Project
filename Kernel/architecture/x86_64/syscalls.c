#include <typedefs.h>
#include <architecture/x86_64/syscalls.h>

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