#ifndef AC3DS_GX_TEXCACHE_H
#define AC3DS_GX_TEXCACHE_H

#include <citro3d.h>
#include <stdbool.h>
#include <stdint.h>

#include "gx_state.h"

typedef struct {
    C3D_Tex* tex;
    float u_scale; /* original size / padded size */
    float v_scale;
} GxtcBinding;

bool gxtc_init(void);
void gxtc_shutdown(void);

/* `save` gets linear 0xRRGGBBAA texels, row 0 at the top; NULL hooks: off; kept across gxtc_init. */
typedef struct {
    const void* data;
    const void* tlut;
    uint32_t content_hash;
    uint8_t format;
    uint32_t width, height;
} GxtcDumpInfo;
typedef bool (*GxtcDumpWantFn)(const GxtcDumpInfo* info);
typedef void (*GxtcDumpSaveFn)(const GxtcDumpInfo* info, const uint32_t* texels);
void gxtc_set_dump_hooks(GxtcDumpWantFn want, GxtcDumpSaveFn save);
void gxtc_frame_begin(void);
/* At each other render target's pass: emu64 may rewind its texture conversion buffers. */
void gxtc_pass_begin(void);

const GxtcBinding* gxtc_get(const GxbTexObj* obj, const GxbTlut* tlut);

uint32_t gxtc_uploads_this_frame(void);
uint64_t gxtc_upload_ticks_this_frame(void);
/* Counts every upload: when it changes, a cached binding may point at new texels. */
uint32_t gxtc_upload_serial(void);
uint32_t gxtc_entry_count(void);
uint32_t gxtc_bytes(void);

#endif /* AC3DS_GX_TEXCACHE_H */
