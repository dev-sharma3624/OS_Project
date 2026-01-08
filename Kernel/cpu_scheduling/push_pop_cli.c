#include <cpu_scheduling/push_pop_cli.h>

#define CLI asm volatile("cli")
#define STI asm volatile("sti")

volatile int n_cli = 0;
volatile int intrpt_enabled = 0;

void push_cli(){
    int is_intrpt_enabled = get_intrpt_flag();

    CLI;

    n_cli++;

    if((n_cli - 1) == 0){
        intrpt_enabled = is_intrpt_enabled;
    }

}

void pop_cli(){
    int is_intrpt_enabled = get_intrpt_flag();
    
    if(is_intrpt_enabled){
        while(1);
    }

    n_cli--;

    if(n_cli < 0){
        while(1);
    }

    if(n_cli == 0 && intrpt_enabled){
        STI;
    }
}