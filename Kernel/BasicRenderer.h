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

extern BasicRenderer globalRenderer;

void BasicRenderer_Init(FrameBuffer* frameBuffer, PSF1_FONT* font, unsigned int color, unsigned int clearColor);
void BasicRenderer_PutChar(char c);
void BasicRenderer_ClearScreen();
void BasicRenderer_NextLine();
void BasicRenderer_Print(const char* c);