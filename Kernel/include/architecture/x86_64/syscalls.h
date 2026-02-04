#include <typedefs.h>

void sys_read(char* buffer);
void sys_print(char* msg);
void sys_exit(int code);
void* sys_sbrk(int64_t increment);
void* malloc(uint64_t size);
void sys_create_file(char* name, char* content);