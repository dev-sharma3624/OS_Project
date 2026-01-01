#pragma once
#include "../../boot_info.h"

typedef unsigned long long int uint64;
typedef unsigned int uint32;

typedef struct{
    unsigned int x;
    unsigned int y;
} point_t;

typedef struct{
    point_t cursor_position;
    frame_buffer_t* frame_buffer;
    psf1_font_t* font;
    unsigned int color;
    unsigned int clear_color;
} basic_renderer_t;

extern basic_renderer_t global_renderer;

void font_renderer_init(frame_buffer_t* frame_buffer, psf1_font_t* font, unsigned int color, unsigned int clear_color);
void font_renderer_put_char(char c);
void font_renderer_clear_screen();
void font_renderer_next_line();
void font_renderer_print(const char* c);