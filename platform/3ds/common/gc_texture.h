#ifndef AC3DS_GC_TEXTURE_H
#define AC3DS_GC_TEXTURE_H

/* GameCube (GX) texture decoding to linear RGBA8: row 0 at the top, 0xRRGGBBAA
 * in a little-endian u32 (PICA200 GPU_RGBA8). */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* Values match GXTexFmt. */
typedef enum {
    GC_TF_I4 = 0x0,
    GC_TF_I8 = 0x1,
    GC_TF_IA4 = 0x2,
    GC_TF_IA8 = 0x3,
    GC_TF_RGB565 = 0x4,
    GC_TF_RGB5A3 = 0x5,
    GC_TF_RGBA8 = 0x6,
    GC_TF_C4 = 0x8,
    GC_TF_C8 = 0x9,
    GC_TF_CMPR = 0xE,
} GcTexFormat;

/* Values match GXTlutFmt. */
typedef enum {
    GC_TLUT_IA8 = 0,
    GC_TLUT_RGB565 = 1,
    GC_TLUT_RGB5A3 = 2,
} GcTlutFormat;

#define GC_PALETTE_MAX_ENTRIES 256

/* `big_endian`: entries still in disc byte order (ARAM/JSystem data), not yet
 * swapped like SWAP_U16 assets (emu64 reports those via pc_gx_tlut_set_native_le). */
typedef struct {
    const void* data;
    GcTlutFormat format;
    uint16_t count;
    bool big_endian;
} GcPalette;

uint32_t gc_decode_rgb5a3(uint16_t value);
uint32_t gc_decode_rgb565(uint16_t value);

/* Includes block padding; 0 for an unknown format. */
size_t gc_texture_size(GcTexFormat format, uint32_t width, uint32_t height);

/* Missing palette entries decode as a grey ramp, as on PC. False (dst
 * untouched) on bad arguments or a C4/C8 texture without a palette. */
bool gc_texture_decode(uint32_t* dst, const void* src, uint32_t width, uint32_t height,
                       GcTexFormat format, const GcPalette* palette);

#endif /* AC3DS_GC_TEXTURE_H */
