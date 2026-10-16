#include "quest_bits.h"

#include <stdio.h>
#include <string.h>

static int failures = 0;

#define CHECK(cond)                                                         \
    do {                                                                    \
        if (!(cond)) {                                                      \
            fprintf(stderr, "%s:%d: CHECK failed: %s\n", __FILE__, __LINE__, \
                    #cond);                                                 \
            failures++;                                                     \
        }                                                                   \
    } while (0)

static const uint8_t GC_BYTES[4] = { 0x95, 0xCC, 0x1E, 0x2D };

/* The same fields as ARM GCC packs them: LSB first in a little-endian 16-bit unit. */
static const uint8_t ARM_BYTES[4] = { 0x56, 0x33, 0x1E, 0x2D };

static void test_gc_to_native(void)
{
    uint8_t b[4];
    memcpy(b, GC_BYTES, sizeof(b));
    quest_bits_repack(b, true);
    CHECK(memcmp(b, ARM_BYTES, sizeof(b)) == 0);
}

static void test_native_to_gc(void)
{
    uint8_t b[4];
    memcpy(b, ARM_BYTES, sizeof(b));
    quest_bits_repack(b, false);
    CHECK(memcmp(b, GC_BYTES, sizeof(b)) == 0);
}

static void test_time_limit_bytes_are_left_alone(void)
{
    uint8_t b[4] = { 0xFF, 0xFF, 0x3B, 0x3B };
    quest_bits_repack(b, true);
    CHECK(b[2] == 0x3B && b[3] == 0x3B);
    CHECK(b[0] == 0xFF && b[1] == 0xFF);
}

static void test_round_trip_every_value(void)
{
    for (uint32_t v = 0; v <= 0xFFFF; v++) {
        uint8_t b[2] = { (uint8_t)(v >> 8), (uint8_t)v };
        quest_bits_repack(b, true);
        quest_bits_repack(b, false);
        if (b[0] != (uint8_t)(v >> 8) || b[1] != (uint8_t)v) {
            CHECK(!"round trip");
            return;
        }
    }
}

int main(void)
{
    test_gc_to_native();
    test_native_to_gc();
    test_time_limit_bytes_are_left_alone();
    test_round_trip_every_value();
    if (failures == 0) {
        printf("quest_bits: all tests passed\n");
    }
    return failures == 0 ? 0 : 1;
}
