#ifndef _COVERFLOW_H_
#define _COVERFLOW_H_

#include <stdint.h>

typedef struct{
    unsigned char red;
    unsigned char green;
    unsigned char blue;
    unsigned char alpha;
}pixel;

#pragma pack(push, 1)
typedef struct {
    uint16_t type;           // File type
    uint32_t size;           // Size of the file
    uint16_t reserved1;      // Reserved field 1
    uint16_t reserved2;      // Reserved field 2
    uint32_t offset;         // Offset to bitmap data
    uint32_t header_size;    // Size of the header
    int32_t  width;          // Width of the image
    int32_t  height;         // Height of the image
    uint16_t planes;         // Number of color planes
    uint16_t bits_per_pixel; // Bits per pixel
    uint32_t compression;    // Compression type
    uint32_t image_size;     // Size of the image data
    int32_t  x_pixels_per_meter; // Horizontal resolution
    int32_t  y_pixels_per_meter; // Vertical resolution
    uint32_t colors_used;    // Number of colors in the color palette
    uint32_t colors_important; // Number of important colors
} BMPHeader;
#pragma pack(pop)

#endif