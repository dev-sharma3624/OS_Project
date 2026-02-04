#include <architecture/x86_64/syscalls.h>
#include <libs/k_string.h>

#define MAX_COMMAND_BUFFER 256
static  command_buffer[MAX_COMMAND_BUFFER];
static int buffer_position = 0;

void kernel_clear_buffer(){
    for(int i = 0; i < MAX_COMMAND_BUFFER; i++){
        command_buffer[i] = 0;
    }
    buffer_position = 0;
}

void kernel_execute_command(){
    
    sys_print("\n");

    if(k_strcmp(command_buffer, "help") == 0){
        sys_print("Available commands:\n");
        sys_print(" - help: Show this menu\n");
        sys_print(" - clear: Clean the screen\n");
        sys_print(" - reboot: Reboot the CPU\n");
        sys_print(" - meminfo: See how much RAM is being used\n");
        sys_print(" - drums of liberation: Awaken the Sun God!\n");
    }

    else if(k_strcmp(command_buffer, "clear") == 0){
        font_renderer_clear_screen();
    }

    else if(k_strcmp(command_buffer, "meminfo") == 0){
        kernel_print_memory_info();
    }

    else if(k_strcmp(command_buffer, "drums of liberation") == 0){
        sys_print("THE ONE PIECE IS REAL!\n");
    }

    else if (buffer_position > 0){
        sys_print("Unknown command: ");
        sys_print(command_buffer);
        sys_print("\n");
    };

    kernel_clear_buffer();
    sys_print("Project D> ");
}

void user_shell_main(){
    
    char input_char;

    sys_print("\n====================================\n");
    sys_print("      Project D v1.0 (USER)      \n");
    sys_print("====================================\n");

    sys_print("Project D v1.0. Type 'help'.\nProject D> ");
    
    while (1)
    {
        sys_read(&input_char);

        if(input_char == '\b'){ //backspace key

            if(buffer_position > 0){

                buffer_position--;
                command_buffer[buffer_position] = 0;

                sys_print("\b");
            }
            continue;
        }

        if (input_char == '\n'){ //enter key
            kernel_execute_command();
            continue;
        }

        if(input_char >= 32 && input_char <= 126){

            if(input_char != 0){

                if(buffer_position < MAX_COMMAND_BUFFER - 1){

                    command_buffer[buffer_position] = input_char;
                    buffer_position++;

                    char temp[2] = {input_char, '\0'};
                    sys_print(temp);

                }
            }
        }

    }
}

/* void user_shell_main() {
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
} */