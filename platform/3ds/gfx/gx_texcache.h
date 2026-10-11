#ifndef AC3DS_GX_TEXCACHE_H
#define AC3DS_GX_TEXCACHE_H

/* Texture cache: GX texture objects (+ palette) decoded once to RGBA8 PICA200
 * textures (decode -> power-of-two pad by wrap mode -> 8x8 Morton tiling, rows
 * flipped so GX texcoords work unchanged). Keyed on pointer, size, format and
 * palette plus content hashes, so textures rewritten in place are re-uploaded. */

#include <citro3d.h>
#include <stdbool.h>
#include <stdint.h>

#include "gx_state.h"

typedef struct {
    C3D_Tex* tex;
    float u_scale; /* original width / padded width */
    float v_scale; /* original height / padded height */
} GxtcBinding;

bool gxtc_init(void);
void gxtc_shutdown(void);

/* The texture dump's hooks (game/texdump_3ds.c): the first time an entry is
 * used, `want` says whether it's new to the dump; if so its texels (linear
 * 0xRRGGBBAA, row 0 at the top) go to `save`. NULLs: off. Kept across gxtc_init. */
typedef struct {
    const void* data;      /* the texture's address in memory */
    const void* tlut;      /* its palette's, or NULL */
    uint32_t content_hash; /* of its texels and palette */
    uint8_t format;
    uint32_t width, height;
} GxtcDumpInfo;
typedef bool (*GxtcDumpWantFn)(const GxtcDumpInfo* info);
typedef void (*GxtcDumpSaveFn)(const GxtcDumpInfo* info, const uint32_t* texels);
void gxtc_set_dump_hooks(GxtcDumpWantFn want, GxtcDumpSaveFn save);
void gxtc_frame_begin(void);
/* Another render target's pass: it runs emu64 again, which may rewind its
 * texture conversion buffers, so this frame's lookup memo starts again. */
void gxtc_pass_begin(void);

/* Decodes and uploads on a miss; NULL if it can't be decoded. */
const GxtcBinding* gxtc_get(const GxbTexObj* obj, const GxbTlut* tlut);

uint32_t gxtc_uploads_this_frame(void);
uint64_t gxtc_upload_ticks_this_frame(void);
/* Counts every upload: when it changes, a cached binding may point at new texels. */
uint32_t gxtc_upload_serial(void);
uint32_t gxtc_entry_count(void);
uint32_t gxtc_bytes(void);

#endif /* AC3DS_GX_TEXCACHE_H */
