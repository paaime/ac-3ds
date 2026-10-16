#include "tex_memo.h"

#include <string.h>

/* MurmurHash3's 32-bit variant: its final mix spreads 32-byte aligned addresses. */
#define MURMUR_C1    0xCC9E2D51u
#define MURMUR_C2    0x1B873593u
#define MURMUR_ADD   0xE6546B64u
#define MURMUR_F1    0x85EBCA6Bu
#define MURMUR_F2    0xC2B2AE35u
#define MURMUR_SEED  0x9747B28Cu
#define WORD_BYTES   4u
#define SLOT_MASK    (TEX_MEMO_SLOTS - 1u)

static uint32_t rotl(uint32_t x, unsigned r)
{
    return (x << r) | (x >> (32u - r));
}

static uint32_t scramble(uint32_t k)
{
    return rotl(k * MURMUR_C1, 15u) * MURMUR_C2;
}

uint32_t tex_memo_hash(const void* data, size_t bytes)
{
    const uint8_t* p = data;
    uint32_t h = MURMUR_SEED;
    size_t words = bytes / WORD_BYTES;
    for (size_t i = 0; i < words; i++) {
        uint32_t k;
        memcpy(&k, p + i * WORD_BYTES, WORD_BYTES); /* any alignment */
        h = rotl(h ^ scramble(k), 13u) * 5u + MURMUR_ADD;
    }
    uint32_t tail = 0;
    for (size_t i = words * WORD_BYTES; i < bytes; i++) {
        tail |= (uint32_t)p[i] << (8u * (i % WORD_BYTES));
    }
    h ^= scramble(tail) ^ (uint32_t)bytes;
    h ^= h >> 16;
    h *= MURMUR_F1;
    h ^= h >> 13;
    h *= MURMUR_F2;
    return h ^ (h >> 16);
}

int tex_memo_get(const TexMemoSlot slots[TEX_MEMO_SLOTS], uint32_t ident, uint32_t frame)
{
    const TexMemoSlot* s = &slots[ident & SLOT_MASK];
    if (frame == 0 || s->frame != frame || s->ident != ident) {
        return TEX_MEMO_NONE;
    }
    return s->entry;
}

void tex_memo_put(TexMemoSlot slots[TEX_MEMO_SLOTS], uint32_t ident, uint32_t frame, int entry)
{
    if (frame == 0 || entry < 0) {
        return;
    }
    slots[ident & SLOT_MASK] = (TexMemoSlot){ ident, frame, entry };
}
