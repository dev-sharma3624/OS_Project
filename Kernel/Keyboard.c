#include "Keyboard.h"
#include "IO.h"

#define KEYBOARD_DATA_PORT 0x60
#define BUFFER_SIZE 256

unsigned char keyboardBuffer[BUFFER_SIZE];
volatile int writeIndex = 0;
volatile int readIndex = 0;

__attribute__((interrupt)) void KeyboardHandler(InterruptFrame* frame){

    unsigned char scanCode = inB(KEYBOARD_DATA_PORT);

    int nextWriteIndex = (writeIndex + 1) % BUFFER_SIZE;

    if(readIndex != nextWriteIndex){
        keyboardBuffer[writeIndex] = scanCode;
        writeIndex = nextWriteIndex;
    }

    outB(0x20, 0x20);

}


unsigned char ReadKey(){
    if(readIndex == writeIndex){
        return 0;
    }

    unsigned char scanCode = keyboardBuffer[readIndex];
    readIndex = (readIndex + 1) % BUFFER_SIZE;
    return scanCode;
}