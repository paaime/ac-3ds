#include "gx_tev_cache.h"

#include <stddef.h>
#include <string.h>

#define WORD_BYTES  4u
#define FNV_OFFSET  2166136261u /* FNV-1a, a word at a time */
#define FNV_PRIME   16777619u
#define MIX_1       0x85EBCA6Bu /* MurmurHash3's final mix */
#define MIX_2       0xC2B2AE35u
#define SLOT_MASK   (GX_TEV_CACHE_SLOTS - 1u)

_Static_assert((GX_TEV_CACHE_SLOTS & (GX_TEV_CACHE_SLOTS - 1)) == 0, "GX_TEV_CACHE_SLOTS: a power of two");

void gx_tev_cache_clear(GxTevCache* cache)
{
    memset(cache, 0, sizeof(*cache));
}

static size_t used_stages(int stage_count)
{
    if (stage_count < 0) {
        return 0;
    }
    return stage_count > GX_TEV_MAX_STAGES ? GX_TEV_MAX_STAGES : (size_t)stage_count;
}

static void make_key(const GxTevConfig* gx, GxTevConfig* key)
{
    memset(key, 0, sizeof(*key));
    key->stage_count = gx->stage_count;
    memcpy(key->stages, gx->stages, used_stages(gx->stage_count) * sizeof(gx->stages[0]));
    memcpy(key->reg, gx->reg, sizeof(key->reg));
    memcpy(key->konst, gx->konst, sizeof(key->konst));
}

static uint32_t hash_bytes(uint32_t h, const uint8_t* p, size_t n)
{
    size_t words = n / WORD_BYTES;
    for (size_t i = 0; i < words; i++) {
        uint32_t w;
        memcpy(&w, p + i * WORD_BYTES, WORD_BYTES);
        h = (h ^ w) * FNV_PRIME;
    }
    for (size_t i = words * WORD_BYTES; i < n; i++) {
        h = (h ^ p[i]) * FNV_PRIME;
    }
    return h;
}

static bool same_setup(const GxTevConfig* key, const GxTevConfig* gx)
{
    return key->stage_count == gx->stage_count &&
           memcmp(key->stages, gx->stages, used_stages(gx->stage_count) * sizeof(gx->stages[0])) == 0 &&
           memcmp(key->reg, gx->reg, sizeof(gx->reg)) == 0 && memcmp(key->konst, gx->konst, sizeof(gx->konst)) == 0;
}

static uint32_t hash_setup(const GxTevConfig* gx)
{
    uint32_t h = FNV_OFFSET ^ (uint32_t)gx->stage_count;
    h = hash_bytes(h, (const uint8_t*)gx->stages, used_stages(gx->stage_count) * sizeof(gx->stages[0]));
    h = hash_bytes(h, (const uint8_t*)gx->reg, sizeof(gx->reg));
    h = hash_bytes(h, (const uint8_t*)gx->konst, sizeof(gx->konst));
    h ^= h >> 16;
    h *= MIX_1;
    h ^= h >> 13;
    h *= MIX_2;
    return h ^ (h >> 16);
}

const PicaCombiner* gx_tev_cache_get(GxTevCache* cache, const GxTevConfig* gx, GxTevTranslateFn translate,
                                     const char** reason)
{
    GxTevCacheSlot* slot = &cache->slots[hash_setup(gx) & SLOT_MASK];
    if (slot->used && same_setup(&slot->key, gx)) {
        cache->hits++;
        return &slot->combiner;
    }
    cache->misses++;
    PicaCombiner combiner;
    if (!translate(gx, &combiner, reason)) {
        return NULL;
    }
    slot->used = true;
    make_key(gx, &slot->key);
    slot->combiner = combiner;
    return &slot->combiner;
}
