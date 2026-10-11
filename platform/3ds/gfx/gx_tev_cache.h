#ifndef AC3DS_GX_TEV_CACHE_H
#define AC3DS_GX_TEV_CACHE_H

/* Remembered TEV translations, keyed by the setup's
 * meaningful part (stage count, used stages, colour registers, konst colours),
 * direct-mapped and checked byte for byte. Failed translations aren't kept. */

#include <stdbool.h>
#include <stdint.h>

#include "gx_tev.h"

#define GX_TEV_CACHE_SLOTS 64

typedef bool (*GxTevTranslateFn)(const GxTevConfig* gx, PicaCombiner* out, const char** reason);

typedef struct {
    bool used;
    GxTevConfig key; /* the setup with its unused stages zeroed */
    PicaCombiner combiner;
} GxTevCacheSlot;

typedef struct {
    GxTevCacheSlot slots[GX_TEV_CACHE_SLOTS];
    uint32_t hits, misses;
} GxTevCache;

void gx_tev_cache_clear(GxTevCache* cache);

/* NULL if it can't be translated (then `reason`, if not NULL, says why). The
 * result stays valid until the next call. */
const PicaCombiner* gx_tev_cache_get(GxTevCache* cache, const GxTevConfig* gx, GxTevTranslateFn translate,
                                     const char** reason);

#endif /* AC3DS_GX_TEV_CACHE_H */
