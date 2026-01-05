#include <typedefs.h>
#include <architecture/x86_64/interrupt_handlers.h>
#include <architecture/x86_64/io.h>
#include <drivers/timer.h>

#define PIT_CMD_PORT 0x43
#define PIT_CH0_PORT 0x40
#define PIC_EOI_PORT 0x20

static volatile uint64_t ticks = 0;

void timer_init(uint32_t freq){

    uint32_t divisor = 1193180 / freq;

    io_out_b(PIT_CMD_PORT, 0x36);

    io_out_b(PIT_CH0_PORT, (uint8_t)(divisor & 0xFF));
    io_out_b(PIT_CH0_PORT, (uint8_t)((divisor >> 8) & 0xFF));

}

void timer_sleep(uint64_t time_in_ms){
    uint64_t end_ticks = ticks + time_in_ms;
    
    while(ticks < end_ticks) {
        asm volatile("hlt");
    }
}

__attribute__((interrupt)) void timer_handler(interrupt_frame_t* frame) {
    ticks++;
    io_out_b(PIC_EOI_PORT, 0x20);
}