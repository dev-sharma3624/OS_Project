#include <typedefs.h>
#include <architecture/x86_64/interrupt_frame.h>

__attribute__((interrupt)) void page_fault_handler(interrupt_frame_t* frame, uint64_t error_code);