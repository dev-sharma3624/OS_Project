#pragma once

void io_out_b(unsigned short int port, unsigned char value);

unsigned char io_in_b(unsigned short int port);

void io_wait();

int io_init();

void io_print(const char* str);
void print_address_hex(void* addr);
void print_dec(long long n);