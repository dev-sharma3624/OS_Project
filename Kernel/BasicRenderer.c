#include "BasicRenderer.h"

BasicRenderer globalRenderer;

void BasicRenderer_PutPixel(uint32 x, uint32 y, uint32 color){

    // pointing pxlPtr to the same address as frameBufferBase but,
    // when accessed through pxlPtr now it will only read 4 bytes at at time
    // this is because each pixel is represented by 4 bytes (RGB and reserved)
    uint32* pxlPtr = (uint32*)globalRenderer.frameBuffer->frameBufferBase;

    /* 
    calculation to reach the desired pixel address:
    

    multiply y co-ordinate with pixelPerScanLine (number of pixels in 
    each row of screen) to move downward to the required line.
    
    add x co-ordinate to move forward from left -> right, to the required pixel.

    add that value to the pxlPtr to move it ahead in the memory by that many bytes
    */
    pxlPtr += x + ( y * globalRenderer.frameBuffer->pixelPerScanLine);

    *pxlPtr = color;
}

void BasicRenderer_Init(FrameBuffer* frameBuffer, PSF1_FONT* font, unsigned int color, unsigned int clearColor){
    globalRenderer.frameBuffer = frameBuffer;
    globalRenderer.font = font;
    globalRenderer.color = color;
    globalRenderer.clearColor = clearColor;
    globalRenderer.cursorPosition.x = 0;
    globalRenderer.cursorPosition.y = 0;
}

void BasicRenderer_ClearScreen(){
    uint64 fBase = (uint64)globalRenderer.frameBuffer->frameBufferBase;
    uint64 bytesPerScanLine = globalRenderer.frameBuffer->pixelPerScanLine * 4; // because each pixel is represent by 4 bytes
    uint64 fHeight = globalRenderer.frameBuffer->screenHeight;

    for(uint64 verticalLine = 0; verticalLine < fHeight; verticalLine++){

        // to move downward to first pixel of the next line with every iteration of outer loop
        uint64 lineBasePtr = fBase + (verticalLine * bytesPerScanLine);

        // same thing as BasicRenderer_PutPixel
        uint32* currentPixel = (uint32*) lineBasePtr;

        // to get the address of the last pixel
        uint32* endOfLine = currentPixel + globalRenderer.frameBuffer->pixelPerScanLine;

        /*
        < works because pixelPerScanLine represents no of pixels "including" the first pixel,
        so endOfLine will point to the exact next 4 bytes after the last pixel as we're adding
        pixelPerScanLine to the current pixel address

        for e.g:
        first pixel address = 0x00
        pixel per scan line = 10
        end of line = 0x28

        but if see the pixels as a series:
        0x0 (first), 0x04 (second), 0x08...., 0x24(last pixel)
        */
        while(currentPixel < endOfLine){ 
            *currentPixel = globalRenderer.clearColor;
            currentPixel++;
        }

    }

    // setting the cursor position to the first pixel (top-right corner)
    globalRenderer.cursorPosition.x = 0;
    globalRenderer.cursorPosition.y = 0;

}

void BasicRenderer_Backspace(){
    unsigned int currentX = globalRenderer.cursorPosition.x;
    unsigned int currentY = globalRenderer.cursorPosition.y;

    globalRenderer.cursorPosition.x -= 8;

    for(long int y = 0; y < 16; y++){
        for(long int x = 0; x < 8; x++){
            BasicRenderer_PutPixel(
                    globalRenderer.cursorPosition.x + x, // 0 + offset to pixel in current line
                    globalRenderer.cursorPosition.y + y, // 0 + offset to current line
                    globalRenderer.clearColor
                );
        }
    }
}


void BasicRenderer_PutChar(char c){

    if(c == '\n'){ // moving to next line if character is EOL
        BasicRenderer_NextLine();
        return;
    }

    if(c == '\b'){
        BasicRenderer_Backspace();
        return;
    }

    // same thing as BasicRenderer_PutPixel
    uint32* pixelPtr = (uint32*)globalRenderer.frameBuffer->frameBufferBase;

    /* 
    usigned char can also be used in place of char, it wouldn't make a difference since we're not actually
    manipulating the data that is present there but only using it for comparison
    */
    char* fontPtr = (char*)globalRenderer.font->glyphBuffer + ( c * globalRenderer.font->header->charSize);

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
            if( *fontPtr & (0b10000000 >> x)){

                BasicRenderer_PutPixel(
                    globalRenderer.cursorPosition.x + x, // 0 + offset to pixel in current line
                    globalRenderer.cursorPosition.y + y, // 0 + offset to current line
                    globalRenderer.color
                );

            }

        }

        // move font pointer to the next byte (next row, downwards)
        fontPtr++;

    }

    // moving cursor position rightwards by 8 points (width of each character in psf1 font)
    globalRenderer.cursorPosition.x += 8;

    // if after moving the cursor position ahead makes the cursor value greater than screen width
    // then that means we do not have enough space in the current line for next character so move down
    // to next row
    if(globalRenderer.cursorPosition.x + 8 > globalRenderer.frameBuffer->screenWidth){
        BasicRenderer_NextLine();
    }

}


void BasicRenderer_NextLine(){
    globalRenderer.cursorPosition.x = 0;
    globalRenderer.cursorPosition.y += 16; // moving 16 lines down as every character is 16 bytes in height
}



void BasicRenderer_Print(const char* str){
    char* chr = (char*) str;

    while(*chr != 0){
        BasicRenderer_PutChar(*chr);
        chr++;
    }
}