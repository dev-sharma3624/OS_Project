#include <architecture/x86_64/io.h>
#include <typedefs.h>

#define COM1_PORT 0x3F8 

void io_out_b(unsigned short int port, unsigned char value){
    __asm__ volatile ("outb %0, %1" : : "a"(value), "Nd"(port));
}

unsigned char io_in_b(unsigned short int port){
    unsigned char returnVal;
    __asm__ volatile ("inb %1, %0" : "=a"(returnVal) : "Nd"(port));
    return returnVal;
}

uint16_t io_in_w(uint16_t port) {
    uint16_t result;
    __asm__ volatile("inw %1, %0" : "=a" (result) : "Nd" (port));
    return result;
}


void io_out_w(uint16_t port, uint16_t data) {
    __asm__ volatile("outw %0, %1" : : "a" (data), "Nd" (port));
}

uint32_t io_in_l(uint16_t port) {
    uint32_t result;
    __asm__ volatile("inl %1, %0" : "=a" (result) : "Nd" (port));
    return result;
}

void io_out_l(uint16_t port, uint32_t data) {
    __asm__ volatile("outl %0, %1" : : "a" (data), "Nd" (port));
}

void io_wait(){
    __asm__ volatile ("outb %%al, $0x80" : : "a"(0));
}

int io_init() {
   io_out_b(COM1_PORT + 1, 0x00);    // Disable all interrupts
   io_out_b(COM1_PORT + 3, 0x80);    // Enable DLAB (set baud rate divisor)
   io_out_b(COM1_PORT + 0, 0x03);    // Set divisor to 3 (lo byte) 38400 baud
   io_out_b(COM1_PORT + 1, 0x00);    //                  (hi byte)
   io_out_b(COM1_PORT + 3, 0x03);    // 8 bits, no parity, one stop bit
   io_out_b(COM1_PORT + 2, 0xC7);    // Enable FIFO, clear them, with 14-byte threshold
   io_out_b(COM1_PORT + 4, 0x0B);    // IRQs enabled, RTS/DSR set
   return 0;
}

void io_serial_putc(char c) {
   while ((io_in_b(COM1_PORT + 5) & 0x20) == 0); // Wait for transmit empty
   io_out_b(COM1_PORT, c);
}

void io_print(const char* str) {
    for (int i = 0; str[i] != 0; i++) {
        io_serial_putc(str[i]);
    }
}

void print_dec(long long n) {
    char buffer[21]; // Large enough for 64-bit int
    int i = 0;
    int is_neg = 0;

    if (n == 0) {
        io_print("0");
        return;
    }

    if (n < 0) {
        is_neg = 1;
        n = -n;
    }

    // Convert digits in reverse
    while (n > 0) {
        buffer[i++] = (n % 10) + '0';
        n /= 10;
    }

    if (is_neg) {
        buffer[i++] = '-';
    }

    buffer[i] = '\0'; // Null terminate

    // Reverse the string
    int start = 0;
    int end = i - 1;
    while (start < end) {
        char temp = buffer[start];
        buffer[start] = buffer[end];
        buffer[end] = temp;
        start++;
        end--;
    }

    io_print(buffer);
    io_print("\n");
}

void print_address_hex(void* addr) {
    uint64_t value = (uint64_t)addr;
    char buffer[19]; // Enough for "0x" + 16 hex digits + null terminator
    
    buffer[0] = '0';
    buffer[1] = 'x';
    buffer[18] = '\0'; // Null-terminate the string

    char* hex_chars = "0123456789ABCDEF";

    // Loop from the last digit (index 17) backwards to index 2
    for (int i = 17; i >= 2; i--) {
        // Get the last 4 bits (nibble)
        int nibble = value & 0xF; 
        
        // Convert to hex character
        buffer[i] = hex_chars[nibble];
        
        // Shift right to process the next nibble
        value >>= 4;
    }

    // Print the final string
    io_print(buffer);
    io_print("\n");
}