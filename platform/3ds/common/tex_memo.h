#ifndef AC3DS_TEX_MEMO_H
#define AC3DS_TEX_MEMO_H

/* Relies on the game not writing textures mid-frame. Frames count from 1 (0 is "never"). */

#include <stddef.h>
#include <stdint.h>

#define TEX_MEMO_SLOTS 512 /* a power of two; a busy town frame binds ~130 textures */
#define TEX_MEMO_NONE  (-1)

typedef struct {
    uint32_t ident;
    uint32_t frame;
    int32_t entry;
} TexMemoSlot;

/* Of an identity struct with its padding zeroed. */
uint32_t tex_memo_hash(const void* data, size_t bytes);

int tex_memo_get(const TexMemoSlot slots[TEX_MEMO_SLOTS], uint32_t ident, uint32_t frame);
void tex_memo_put(TexMemoSlot slots[TEX_MEMO_SLOTS], uint32_t ident, uint32_t frame, int entry);

#endif /* AC3DS_TEX_MEMO_H */
