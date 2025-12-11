#include "BasicRenderer.h"

void BasicRenderer_PutPixel(BasicRenderer* basicRenderer, uint32 x, uint32 y, uint32 color){
    uint32* pxlPtr = (uint32*)basicRenderer->frameBuffer->frameBufferBase;

    pxlPtr += x + ( y * basicRenderer->frameBuffer->pixelPerScanLine);

    *pxlPtr = color;
}

void BasicRenderer_Init(BasicRenderer* basicRenderer, FrameBuffer* frameBuffer, PSF1_FONT* font){
    basicRenderer->frameBuffer = frameBuffer;
    basicRenderer->font = font;
    basicRenderer->color = 0xffffffff;
    basicRenderer->clearColor = 0xff000000;
    basicRenderer->cursorPosition.x = 0;
    basicRenderer->cursorPosition.y = 0;
}

void BasicRenderer_ClearScreen(BasicRenderer* basicRenderer){
    uint64 fBase = (uint64)basicRenderer->frameBuffer->frameBufferBase;
    uint64 bytesPerScanLine = basicRenderer->frameBuffer->pixelPerScanLine * 4;
    uint64 fHeight = basicRenderer->frameBuffer->screenHeight;

    for(uint64 verticalLine = 0; verticalLine < fHeight; verticalLine++){

        uint64 lineBasePtr = fBase + (verticalLine * bytesPerScanLine);

        uint32* currentPixel = (uint32*) lineBasePtr;
        uint32* endOfLine = currentPixel + basicRenderer->frameBuffer->pixelPerScanLine;

        while(currentPixel < endOfLine){
            *currentPixel = basicRenderer->clearColor;
            currentPixel++;
        }

    }

    basicRenderer->cursorPosition.x = 0;
    basicRenderer->cursorPosition.y = 0;

}


void BasicRenderer_PutChar(BasicRenderer* basicRenderer, char c){
    uint32* pixelPtr = (uint32*)basicRenderer->frameBuffer->frameBufferBase;

    char* fontPtr = (char*)basicRenderer->font->glyphBuffer + ( c * basicRenderer->font->header->charSize);

    for(long int y = 0; y < 16; y++){

        for(long int x = 0; x < 8; x++){

            if( *fontPtr & (0b10000000 >> x)){

                BasicRenderer_PutPixel(
                    basicRenderer,
                    basicRenderer->cursorPosition.x + x,
                    basicRenderer->cursorPosition.y + y,
                    basicRenderer->color
                );

            }

        }

        fontPtr++;

    }

    basicRenderer->cursorPosition.x += 8;

    if(basicRenderer->cursorPosition.x + 8 > basicRenderer->frameBuffer->screenWidth){
        BasicRenderer_NextLine(basicRenderer);
    }

}


void BasicRenderer_NextLine(BasicRenderer* basicRenderer){
    basicRenderer->cursorPosition.x = 0;
    basicRenderer->cursorPosition.y += 16;
}



void BasicRenderer_Print(BasicRenderer* basicRenderer, const char* str){
    char* chr = (char*) str;

    while(*chr != 0){
        if(*chr == '\n'){
            BasicRenderer_NextLine(basicRenderer);
        }else{
            BasicRenderer_PutChar(basicRenderer, *chr);
        }
        chr++;
    }
}