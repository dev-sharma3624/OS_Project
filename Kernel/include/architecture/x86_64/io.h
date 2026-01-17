#pragma once
#include <typedefs.h>

void io_out_b(unsigned short int port, unsigned char value);

void io_out_w(uint16_t port, uint16_t data);

void io_out_l(uint16_t port, uint32_t data);

unsigned char io_in_b(unsigned short int port);

uint16_t io_in_w(uint16_t port);

uint32_t io_in_l(uint16_t port);


void io_wait();

int io_init();

void io_print(const char* str);
void print_address_hex(void* addr);
void print_dec(long long n);