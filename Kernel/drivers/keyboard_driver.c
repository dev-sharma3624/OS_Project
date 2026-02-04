#include <drivers/keyboard_driver.h>
#include <architecture/x86_64/io.h>
#include <architecture/x86_64/interrupt_handlers.h>
#include <cpu_scheduling/process.h>
#include <cpu_scheduling/scheduler.h>
#include <drivers/keyboard_map.h>

#define KEYBOARD_DATA_PORT 0x60
#define BUFFER_SIZE 256

unsigned char keyboard_buffer[BUFFER_SIZE];
volatile int write_index = 0;
volatile int read_index = 0;
volatile bool shift_pressed = false;

tcb_t* keyboard_wait_task = NULL;
extern tcb_t* current_task;

void keyboard_driver_handler(){

    unsigned char scan_code = io_in_b(KEYBOARD_DATA_PORT); // read data from hardware port

    int next_write_index = (write_index + 1) % BUFFER_SIZE; // calculate next write index, goes to 0 if exceeds buffer size

    if(read_index != next_write_index){ //buffer not full check
        keyboard_buffer[write_index] = scan_code; //add the value recieved from port to buffer
        write_index = next_write_index; //increment the write_index
    }


    //condition true(keyboard_wait_task is NULL) => buffer is not empty, meaning there are currently keyboard presses that have not been processed but read into buffer
    //condition false(keyboard_wait_task is not NULL) => buffer is empty, in that case we put the current thread(shell thread) to BLOCKED state to allow background processes to execute
    if(keyboard_wait_task != NULL){ 
        keyboard_wait_task->task_state = TASK_READY; //start the current thread(shell thread) to start processing scancodes accumulated in buffer 
        keyboard_wait_task = NULL; //so it can be set to not null again in future if the buffer becomes empty (no keyboard activity)

        schedule();
    }

}

char scancode_ascii_converter(unsigned char scancode){

    // 1. Handle Shift Press (Make Code)
    if (scancode == 0x2A || scancode == 0x36) { // Left or Right Shift
        shift_pressed = 1;
        return 0;
    }

    // 2. Handle Shift Release (Break Code)
    // 0xAA is Break for 0x2A (Left Shift)
    // 0xB6 is Break for 0x36 (Right Shift)
    if (scancode == 0xAA || scancode == 0xB6) { 
        shift_pressed = 0;
        return 0;
    }

    // 3. Ignore other Break Codes (Key releases)
    if (scancode & 0x80) { 
        // Example: 'A' release is 0x9E (0x1E + 0x80)
        return 0; 
    }

    // 4. Translate Character
    char c = 0;
    if (shift_pressed) {

        c = scan_code_for_shift_lookup_table[scancode];
        
    } else {
        c = scan_code_for_lookup_table[scancode];
    }

    if (c != 0) {
        return c;
    }

    return 0;
}

char read_key(){

    // continuos loop to read data
    while(1) {

        if(read_index != write_index){// check to ensure, data is present

            unsigned char scan_code = keyboard_buffer[read_index]; //read the data from current read index
            read_index = (read_index + 1) % BUFFER_SIZE; // increment the read index, set to 0 if exceeds buffer size

            char ascii = scancode_ascii_converter(scan_code);

            if(ascii != 0){
                return ascii;
            }
        }

        //this part below will be executed only if there is no data to process inside buffer (no keyboard activity)

        keyboard_wait_task = current_task; //keyboard_wait_task becomes not NULL => allows check in keyboard_driver_handler() to start the thread if it currently blocked 

        current_task->task_state = TASK_BLOCKED; //since no data present to process, put this thread (shell thread) to BLOCKED state

        schedule(); //pick up another READY state thread from queue until next keyboard interrupt fires
        
    }

}