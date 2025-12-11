#include "BasicRenderer.h"

BasicRenderer globalRenderer;

void BasicRenderer_PutPixel(uint32 x, uint32 y, uint32 color){
    uint32* pxlPtr = (uint32*)globalRenderer.frameBuffer->frameBufferBase;

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
    uint64 bytesPerScanLine = globalRenderer.frameBuffer->pixelPerScanLine * 4;
    uint64 fHeight = globalRenderer.frameBuffer->screenHeight;

    for(uint64 verticalLine = 0; verticalLine < fHeight; verticalLine++){

        uint64 lineBasePtr = fBase + (verticalLine * bytesPerScanLine);

        uint32* currentPixel = (uint32*) lineBasePtr;
        uint32* endOfLine = currentPixel + globalRenderer.frameBuffer->pixelPerScanLine;

        while(currentPixel < endOfLine){
            *currentPixel = globalRenderer.clearColor;
            currentPixel++;
        }

    }

    globalRenderer.cursorPosition.x = 0;
    globalRenderer.cursorPosition.y = 0;

}


void BasicRenderer_PutChar(char c){

    if(c == '\n'){
        BasicRenderer_NextLine();
        return;
    }

    uint32* pixelPtr = (uint32*)globalRenderer.frameBuffer->frameBufferBase;

    char* fontPtr = (char*)globalRenderer.font->glyphBuffer + ( c * globalRenderer.font->header->charSize);

    for(long int y = 0; y < 16; y++){

        for(long int x = 0; x < 8; x++){

            if( *fontPtr & (0b10000000 >> x)){

                BasicRenderer_PutPixel(
                    globalRenderer.cursorPosition.x + x,
                    globalRenderer.cursorPosition.y + y,
                    globalRenderer.color
                );

            }

        }

        fontPtr++;

    }

    globalRenderer.cursorPosition.x += 8;

    if(globalRenderer.cursorPosition.x + 8 > globalRenderer.frameBuffer->screenWidth){
        BasicRenderer_NextLine();
    }

}


void BasicRenderer_NextLine(){
    globalRenderer.cursorPosition.x = 0;
    globalRenderer.cursorPosition.y += 16;
}



void BasicRenderer_Print(const char* str){
    char* chr = (char*) str;

    while(*chr != 0){
        BasicRenderer_PutChar(*chr);
        chr++;
    }
}