#include "quest_bits.h"

/* Field widths in declaration order (include/m_quest.h). */
#define FIELD_COUNT 6
static const uint8_t WIDTH[FIELD_COUNT] = { 2, 6, 1, 4, 1, 2 };
#define UNIT_BITS 16

static uint32_t gc_shift(int field)
{
    uint32_t used = 0;
    for (int i = 0; i <= field; i++) {
        used += WIDTH[i];
    }
    return UNIT_BITS - used;
}

static uint32_t native_shift(int field)
{
    uint32_t used = 0;
    for (int i = 0; i < field; i++) {
        used += WIDTH[i];
    }
    return used;
}

void quest_bits_repack(uint8_t* bytes, bool from_gc)
{
    uint32_t in = from_gc ? ((uint32_t)bytes[0] << 8) | bytes[1] : bytes[0] | ((uint32_t)bytes[1] << 8);
    uint32_t out = 0;
    for (int f = 0; f < FIELD_COUNT; f++) {
        uint32_t mask = (1u << WIDTH[f]) - 1u;
        uint32_t value = (in >> (from_gc ? gc_shift(f) : native_shift(f))) & mask;
        out |= value << (from_gc ? native_shift(f) : gc_shift(f));
    }
    if (from_gc) {
        bytes[0] = (uint8_t)out;
        bytes[1] = (uint8_t)(out >> 8);
    } else {
        bytes[0] = (uint8_t)(out >> 8);
        bytes[1] = (uint8_t)out;
    }
}
