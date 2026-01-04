#include <drivers/keyboard_driver.h>
#include <architecture/x86_64/io.h>
#include <architecture/x86_64/interrupt_handlers.h>

#define KEYBOARD_DATA_PORT 0x60
#define BUFFER_SIZE 256

unsigned char keyboard_buffer[BUFFER_SIZE];
volatile int write_index = 0;
volatile int read_index = 0;

__attribute__((interrupt)) void keyboard_driver_handler(interrupt_frame_t* frame){

    unsigned char scan_code = io_in_b(KEYBOARD_DATA_PORT);

    int next_write_index = (write_index + 1) % BUFFER_SIZE;

    if(read_index != next_write_index){
        keyboard_buffer[write_index] = scan_code;
        write_index = next_write_index;
    }

    io_out_b(0x20, 0x20);

}


unsigned char read_key(){
    if(read_index == write_index){
        return 0;
    }

    unsigned char scan_code = keyboard_buffer[read_index];
    read_index = (read_index + 1) % BUFFER_SIZE;
    return scan_code;
}