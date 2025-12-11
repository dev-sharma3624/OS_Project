#pragma once
#include <boot_info.h>

typedef unsigned long long int uint64;
typedef unsigned int uint32;

typedef struct{
    unsigned int x;
    unsigned int y;
} Point;

typedef struct{
    Point cursorPosition;
    FrameBuffer* frameBuffer;
    PSF1_FONT* font;
    unsigned int color;
    unsigned int clearColor;
} BasicRenderer;

void BasicRenderer_Init(BasicRenderer* basicRenderer, FrameBuffer* frameBuffer, PSF1_FONT* font);
void BasicRenderer_ClearScreen(BasicRenderer* basicRenderer);
void BasicRenderer_PutChar(BasicRenderer* basicRenderer, char c);
void BasicRenderer_NextLine(BasicRenderer* basicRenderer);
void BasicRenderer_Print(BasicRenderer* basicRenderer, const char* c);
void BasicRenderer_PutPixel(BasicRenderer* basicRenderer, uint32 x, uint32 y, uint32 color);