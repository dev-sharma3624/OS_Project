#ifndef BOOT_INFO_H
#define BOOT_INFO_H

typedef unsigned long long size_t;
typedef unsigned int int_t;
typedef unsigned char char_t;

typedef struct boot_info
{
    void* frameBufferBase;
    size_t frameBufferSize;
    int_t screenWidth;
    int_t screenHeight;
    int_t pixelPerScanLine;

    void* font;

    void* mMap;
    size_t mMapSize;
    size_t mMapDescSize;

} BOOT_INFO;


#endif