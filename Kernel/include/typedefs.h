#pragma once

typedef unsigned char      uint8_t;
typedef unsigned short     uint16_t;
typedef unsigned int       uint32_t;
typedef unsigned long long uint64_t;

typedef char      int8_t;
typedef short     int16_t;
typedef int       int32_t;
typedef long long int64_t;

typedef unsigned long long size_t;

#define NULL ((void*)0)

typedef unsigned char bool;

#define true 1
#define false 0

#define KERNEL_VIRT_BASE 0xFFFFFFFF80000000
#define KERNEL_PHSY_BASE 0x8000000
// #define FRAMEBUFFER_VIRT_ADDR 0xFFFFFFFF40000000
#define P2V(a) ((uint64_t)(a) + KERNEL_VIRT_BASE -KERNEL_PHSY_BASE) //convert physical address to virtual
#define V2P(a) ((uint64_t)(a) + KERNEL_PHSY_BASE - KERNEL_VIRT_BASE) //convert virtual address to physical