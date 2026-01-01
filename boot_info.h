#pragma once

typedef struct {
    unsigned char file_identifier[2];
    unsigned char mode;
    unsigned char char_size;
} psf1_header_t;

typedef struct {
    psf1_header_t* header;
    void* glyph_buffer;
} psf1_font_t;



typedef struct{
    void* frame_buffer_base;
    unsigned long long frame_buffer_size;
    unsigned int screen_width;
    unsigned int screen_height;
    unsigned int pixel_per_scan_line;
} frame_buffer_t;


typedef struct{
    frame_buffer_t frame_buffer;

    psf1_font_t* font;

    void* m_map;
    unsigned long long m_map_size;
    unsigned long long m_map_desc_size;

} boot_info_t;