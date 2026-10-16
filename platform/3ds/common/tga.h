#ifndef AC3DS_TGA_H
#define AC3DS_TGA_H

#include <stddef.h>
#include <stdint.h>

#define TGA_HEADER_BYTES 18
#define TGA_MAX_SIDE     1024u /* the largest GX texture; keeps sizes far from a 32-bit wrap */

size_t tga_rgba8_size(uint32_t width, uint32_t height);

/* Texels in 0xRRGGBBAA, row 0 at the top; returns the size, or 0. */
size_t tga_write_rgba8(uint8_t* out, size_t out_size, const uint32_t* texels, uint32_t width, uint32_t height);

#endif /* AC3DS_TGA_H */
