#include "PIC.h"
#include "IO.h"

#define PIC1_COMMAND 0x20
#define PIC1_DATA 0x21
#define PIC2_COMMAND 0xA0
#define PIC2_DATA 0xA1

#define ICW1_INIT 0x11
#define ICW4_8086 0x01

void remapPIC(){

    unsigned char a1 = inB(PIC1_DATA);
    unsigned char a2 = inB(PIC2_DATA);

    outB(PIC1_COMMAND, ICW1_INIT);
    ioWait();
    outB(PIC2_COMMAND, ICW1_INIT);
    ioWait();

    outB(PIC1_DATA, 0x20);
    ioWait();
    outB(PIC2_DATA, 0x28);
    ioWait();

    outB(PIC1_DATA, 4);
    ioWait();
    outB(PIC2_DATA, 2);
    ioWait();

    outB(PIC1_DATA, ICW4_8086);
    ioWait();
    outB(PIC2_DATA, ICW4_8086);
    ioWait();

    outB(PIC1_DATA, 0xFD);
    ioWait();
    outB(PIC2_DATA, 0xFF);

}