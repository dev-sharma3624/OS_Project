#ifndef BOOT_INFO_H
#define BOOT_INFO_H

typedef unsigned long long size_t;
typedef unsigned int int_t;
typedef unsigned char char_t;

typedef struct {
    char_t fileIdentifier[2];
    char_t mode;
    char_t charSize;
} PSF1_HEADER;

typedef struct {
    PSF1_HEADER* header;
    void* glyphBuffer;
} PSF1_FONT;



typedef struct{
    void* frameBufferBase;
    size_t frameBufferSize;
    int_t screenWidth;
    int_t screenHeight;
    int_t pixelPerScanLine;
}  FrameBuffer;


typedef struct{
    FrameBuffer frameBuffer;

    PSF1_FONT* font;

    void* mMap;
    size_t mMapSize;
    size_t mMapDescSize;

} BOOT_INFO;


#endif