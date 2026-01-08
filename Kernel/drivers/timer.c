#include <typedefs.h>
#include <architecture/x86_64/interrupt_handlers.h>
#include <architecture/x86_64/io.h>
#include <drivers/timer.h>
#include <cpu_scheduling/scheduler.h>

#define PIT_CMD_PORT 0x43
#define PIT_CH0_PORT 0x40

volatile uint64_t ticks = 0;

void timer_init(uint32_t freq){

    uint32_t divisor = 1193180 / freq;

    io_out_b(PIT_CMD_PORT, 0x36);

    io_out_b(PIT_CH0_PORT, (uint8_t)(divisor & 0xFF));
    io_out_b(PIT_CH0_PORT, (uint8_t)((divisor >> 8) & 0xFF));

}

void timer_handler() {
    ticks++;
    task_check_wakeup();
    if(ticks % 100 == 0){
        schedule();
    }
}