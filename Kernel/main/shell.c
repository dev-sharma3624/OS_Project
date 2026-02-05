#include <architecture/x86_64/syscalls.h>
#include <libs/k_string.h>

#define MAX_COMMAND_BUFFER 256
static char command_buffer[MAX_COMMAND_BUFFER];
static int buffer_position = 0;

void clear_buffer(){
    for(int i = 0; i < MAX_COMMAND_BUFFER; i++){
        command_buffer[i] = 0;
    }
    buffer_position = 0;
}

void execute_command(){

    char* args[10];
    int argc = str_split(command_buffer, '|', args);

    if(argc <= 0){
        return;
    }

    char* fist_arg = args[0];
    str_trim(fist_arg);
    
    sys_print("\n");

    if(k_strcmp(fist_arg, "help") == 0){
        sys_print("Available commands:\n");
        sys_print(" - help: Show this menu\n");
        sys_print(" - clear: Clean the screen\n");
        sys_print(" - reboot: Reboot the CPU\n");
        sys_print(" - meminfo: See how much RAM is being used\n");
        sys_print(" - create: create a new text file\n");
        sys_print(" - read: read a text file\n");
        sys_print(" - ls: list all files\n");
        sys_print(" - drums of liberation: Awaken the Sun God!\n");
    }

    else if(k_strcmp(fist_arg, "clear") == 0){
        sys_clear();
    }

    else if(k_strcmp(fist_arg, "meminfo") == 0){
        sys_meminfo();
    }

    else if(k_strcmp(fist_arg, "create") == 0){
        char* filename = args[1];
        char* content = args[2];
        str_trim(filename);
        str_trim(content);
        sys_create_file(filename, content);
    }

    else if(k_strcmp(fist_arg, "read") == 0){
        char* filename = args[1];
        str_trim(filename);
        sys_read_file(filename);
    }

    else if(k_strcmp(fist_arg, "ls") == 0){
        sys_ls();
    }

    else if(k_strcmp(fist_arg, "drums of liberation") == 0){
        sys_print("THE ONE PIECE IS REAL!\n");
    }

    else if (buffer_position > 0){
        sys_print("Unknown command: ");
        sys_print(command_buffer);
        sys_print("\n");
    };

    clear_buffer();
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
            execute_command();
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