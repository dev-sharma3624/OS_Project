#include <drivers/font_renderer.h>

basic_renderer_t global_renderer;

void font_renderer_put_pixel(uint32 x, uint32 y, uint32 color){

    // pointing pxl_ptr to the same address as frame_buffer_base but,
    // when accessed through pxl_ptr now it will only read 4 bytes at at time
    // this is because each pixel is represented by 4 bytes (RGB and reserved)
    uint32* pxl_ptr = (uint32*)global_renderer.frame_buffer->frame_buffer_base;

    /* 
    calculation to reach the desired pixel address:
    

    multiply y co-ordinate with pixel_per_scan_line (number of pixels in 
    each row of screen) to move downward to the required line.
    
    add x co-ordinate to move forward from left -> right, to the required pixel.

    add that value to the pxl_ptr to move it ahead in the memory by that many bytes
    */
    pxl_ptr += x + ( y * global_renderer.frame_buffer->pixel_per_scan_line);

    *pxl_ptr = color;
}


void font_renderer_backspace(){
    unsigned int current_x = global_renderer.cursor_position.x;
    unsigned int current_y = global_renderer.cursor_position.y;

    global_renderer.cursor_position.x -= 8;

    for(long int y = 0; y < 16; y++){
        for(long int x = 0; x < 8; x++){
            font_renderer_put_pixel(
                    global_renderer.cursor_position.x + x, // 0 + offset to pixel in current line
                    global_renderer.cursor_position.y + y, // 0 + offset to current line
                    global_renderer.clear_color
                );
        }
    }
}

void font_renderer_init(frame_buffer_t* frame_buffer, psf1_font_t* font, unsigned int color, unsigned int clear_color){
    global_renderer.frame_buffer = frame_buffer;
    global_renderer.font = font;
    global_renderer.color = color;
    global_renderer.clear_color = clear_color;
    global_renderer.cursor_position.x = 0;
    global_renderer.cursor_position.y = 0;
}

void font_renderer_clear_screen(){
    uint64 f_base = (uint64)global_renderer.frame_buffer->frame_buffer_base;
    uint64 bytes_per_scan_line = global_renderer.frame_buffer->pixel_per_scan_line * 4; // because each pixel is represent by 4 bytes
    uint64 f_height = global_renderer.frame_buffer->screen_height;

    for(uint64 vertical_line = 0; vertical_line < f_height; vertical_line++){

        // to move downward to first pixel of the next line with every iteration of outer loop
        uint64 line_base_ptr = f_base + (vertical_line * bytes_per_scan_line);

        // same thing as font_renderer_put_pixel
        uint32* current_pixel = (uint32*) line_base_ptr;

        // to get the address of the last pixel
        uint32* end_of_line = current_pixel + global_renderer.frame_buffer->pixel_per_scan_line;

        /*
        < works because pixel_per_scan_line represents no of pixels "including" the first pixel,
        so end_of_line will point_t to the exact next 4 bytes after the last pixel as we're adding
        pixel_per_scan_line to the current pixel address

        for e.g:
        first pixel address = 0x00
        pixel per scan line = 10
        end of line = 0x28

        but if see the pixels as a series:
        0x0 (first), 0x04 (second), 0x08...., 0x24(last pixel)
        */
        while(current_pixel < end_of_line){
            *current_pixel = global_renderer.clear_color;
            current_pixel++;
        }

    }

    // setting the cursor position to the first pixel (top-right corner)
    global_renderer.cursor_position.x = 0;
    global_renderer.cursor_position.y = 0;

}


void font_renderer_put_char(char c){

    if(c == '\n'){ // moving to next line if character is EOL
        font_renderer_next_line();
        return;
    }

    if(c == '\b'){
        font_renderer_backspace();
        return;
    }

    // same thing as font_renderer_put_pixel
    uint32* pixel_ptr = (uint32*)global_renderer.frame_buffer->frame_buffer_base;

    /* 
    usigned char can also be used in place of char, it wouldn't make a difference since we're not actually
    manipulating the data that is present there but only using it for comparison
    */
    char* font_ptr = (char*)global_renderer.font->glyph_buffer + ( c * global_renderer.font->header->char_size);

    // moving 16 times vertically because every character in psf1 font is 16 bytes long
    for(long int y = 0; y < 16; y++){

        // moving 8 times horizontally because every row of the character in psf1 font is 8 bits (1 byte) long
        for(long int x = 0; x < 8; x++){

            /*
            for e.g:
            say, row 5(index 4) of character A has bit value 0b00111000 in psf1 font 

            on iteration 1, x = 0,
            0b00111000 & 0b10000000 = 0b00000000 which is not true so the first pixel doesn't change.

            on iteration 3, x = 2,
            0b00111000 & 0b00100000 (after two right shifts) = 0b00100000 which is treated as true so,
            pixel 3(x(index) = 2) of row 5(y = 4) changes to the color given
            */
            if( *font_ptr & (0b10000000 >> x)){

                font_renderer_put_pixel(
                    global_renderer.cursor_position.x + x, // 0 + offset to pixel in current line
                    global_renderer.cursor_position.y + y, // 0 + offset to current line
                    global_renderer.color
                );

            }

        }

        // move font pointer to the next byte (next row, downwards)
        font_ptr++;

    }

    // moving cursor position rightwards by 8 points (width of each character in psf1 font)
    global_renderer.cursor_position.x += 8;

    // if after moving the cursor position ahead makes the cursor value greater than screen width
    // then that means we do not have enough space in the current line for next character so move down
    // to next row
    if(global_renderer.cursor_position.x + 8 > global_renderer.frame_buffer->screen_width){
        font_renderer_next_line();
    }

}


void font_renderer_next_line(){
    global_renderer.cursor_position.x = 0;
    global_renderer.cursor_position.y += 16; // moving 16 lines down as every character is 16 bytes in height
}



void font_renderer_print(const char* str){
    char* chr = (char*) str;

    while(*chr != 0){
        font_renderer_put_char(*chr);
        chr++;
    }
}