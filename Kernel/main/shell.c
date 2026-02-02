#include <typedefs.h>
#include <architecture/x86_64/syscalls.h>
#include <libs/k_string.h>

/* #define MAX_COMMAND_BUFFER 256
char command_buffer[MAX_COMMAND_BUFFER];
int buffer_position = 0;

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

void shell(){
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
    
} */

void user_shell_main() {
    char cmd[100];
    int idx = 0;

    sys_print("\n====================================\n");
    sys_print("      ARCHITECT OS v1.0 (USER)      \n");
    sys_print("====================================\n");

    while (1) {
        sys_print("\nArchitect@NVMe:~$ ");
        idx = 0;

        // --- 1. Input Loop ---
        while (1) {
            char c;
            char buf[2]; 
            
            // Read 1 byte from keyboard (Syscall)
            sys_read(&c);

            // Handle Enter
            if (c == '\n') {
                cmd[idx] = '\0';
                break;
            }

            // Echo back to screen so user sees what they type
            buf[0] = c; 
            buf[1] = '\0';
            sys_print(buf);

            // Store in buffer
            if (idx < 99) cmd[idx++] = c;
        }

        sys_print("\n"); // Newline after enter

        // --- 2. Command Parsing ---
        if (k_strcmp(cmd, "help") == 0) {
            sys_print("Commands: ls, exit, help, hello\n");
        } 
        else if (k_strcmp(cmd, "ls") == 0) {
            // THE BIG MOMENT: Ring 3 triggering NVMe Driver
            // Assuming SYS_LS is syscall number 4
            asm volatile ("int $0x80" :: "a"(4)); 
        } 
        else if (k_strcmp(cmd, "hello") == 0) {
            sys_print("Hello from Ring 3! I cannot touch hardware directly.\n");
        }
        else if (k_strcmp(cmd, "exit") == 0) {
            sys_exit(0);
        } 
        else if (idx > 0) {
            sys_print("Unknown command.\n");
        }
    }
}