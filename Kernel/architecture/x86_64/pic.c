#include <architecture/x86_64/pic.h>
#include <architecture/x86_64/io.h>

#define PIC1_COMMAND 0x20
#define PIC1_DATA 0x21
#define PIC2_COMMAND 0xA0
#define PIC2_DATA 0xA1

#define ICW1_INIT 0x11
#define ICW4_8086 0x01

void remap_pic(){

    unsigned char a1 = io_in_b(PIC1_DATA);
    unsigned char a2 = io_in_b(PIC2_DATA);

    io_out_b(PIC1_COMMAND, ICW1_INIT);
    io_wait();
    io_out_b(PIC2_COMMAND, ICW1_INIT);
    io_wait();

    io_out_b(PIC1_DATA, 0x20);
    io_wait();
    io_out_b(PIC2_DATA, 0x28);
    io_wait();

    io_out_b(PIC1_DATA, 4);
    io_wait();
    io_out_b(PIC2_DATA, 2);
    io_wait();

    io_out_b(PIC1_DATA, ICW4_8086);
    io_wait();
    io_out_b(PIC2_DATA, ICW4_8086);
    io_wait();

    io_out_b(PIC1_DATA, 0xFC);
    io_wait();
    io_out_b(PIC2_DATA, 0xFF);

}