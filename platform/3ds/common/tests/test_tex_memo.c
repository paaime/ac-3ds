/* Host tests for tex_memo.c. */
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "../tex_memo.h"

static int failures;
#define CHECK(c)                                                              \
    do {                                                                      \
        if (!(c)) {                                                           \
            fprintf(stderr, "%s:%d: CHECK failed: %s\n", __FILE__, __LINE__, #c); \
            failures++;                                                       \
        }                                                                     \
    } while (0)

static TexMemoSlot s_slots[TEX_MEMO_SLOTS];

static void clear(void)
{
    memset(s_slots, 0, sizeof(s_slots));
}

static void test_empty_table_finds_nothing(void)
{
    clear();
    CHECK(tex_memo_get(s_slots, 0, 1) == TEX_MEMO_NONE);
    CHECK(tex_memo_get(s_slots, 0x12345678u, 1) == TEX_MEMO_NONE);
    CHECK(tex_memo_get(s_slots, 0, 0) == TEX_MEMO_NONE); /* frame 0 is "never" */
}

static void test_finds_what_this_frame_stored(void)
{
    clear();
    tex_memo_put(s_slots, 0xABCD0001u, 7, 42);
    CHECK(tex_memo_get(s_slots, 0xABCD0001u, 7) == 42);
    CHECK(tex_memo_get(s_slots, 0xABCD0002u, 7) == TEX_MEMO_NONE);
}

static void test_forgets_at_the_next_frame(void)
{
    clear();
    tex_memo_put(s_slots, 99u, 7, 3);
    CHECK(tex_memo_get(s_slots, 99u, 8) == TEX_MEMO_NONE);
    CHECK(tex_memo_get(s_slots, 99u, 6) == TEX_MEMO_NONE);
}

static void test_frame_zero_is_never_stored(void)
{
    clear();
    tex_memo_put(s_slots, 5u, 0, 1);
    CHECK(tex_memo_get(s_slots, 5u, 0) == TEX_MEMO_NONE);
}

static void test_a_slot_keeps_the_last_identity(void)
{
    clear();
    uint32_t a = 10u;
    uint32_t b = a + TEX_MEMO_SLOTS; /* the same slot */
    tex_memo_put(s_slots, a, 1, 1);
    tex_memo_put(s_slots, b, 1, 2);
    CHECK(tex_memo_get(s_slots, b, 1) == 2);
    CHECK(tex_memo_get(s_slots, a, 1) == TEX_MEMO_NONE);
}

static void test_negative_entries_are_not_stored(void)
{
    clear();
    tex_memo_put(s_slots, 3u, 1, -5);
    CHECK(tex_memo_get(s_slots, 3u, 1) == TEX_MEMO_NONE);
}

typedef struct {
    const void* data;
    uint16_t width, height;
    uint32_t format;
} Ident;

static void test_hash_is_a_function_of_the_bytes(void)
{
    Ident x, y;
    memset(&x, 0, sizeof(x));
    memset(&y, 0, sizeof(y));
    x.data = y.data = (const void*)(uintptr_t)0x30001000u;
    x.width = y.width = 64;
    x.height = y.height = 32;
    CHECK(tex_memo_hash(&x, sizeof(x)) == tex_memo_hash(&y, sizeof(y)));
    y.format = 1;
    CHECK(tex_memo_hash(&x, sizeof(x)) != tex_memo_hash(&y, sizeof(y)));
    y.format = 0;
    y.height = 64;
    CHECK(tex_memo_hash(&x, sizeof(x)) != tex_memo_hash(&y, sizeof(y)));
}

static void test_hash_reads_a_trailing_partial_word(void)
{
    uint8_t a[6] = { 1, 2, 3, 4, 5, 6 };
    uint8_t b[6] = { 1, 2, 3, 4, 5, 7 };
    CHECK(tex_memo_hash(a, sizeof(a)) != tex_memo_hash(b, sizeof(b)));
    CHECK(tex_memo_hash(a, 5) == tex_memo_hash(b, 5));
}

/* Textures sit at 32-byte aligned addresses: their identities must still
 * spread over the slots, or the memo would keep evicting itself. */
static void test_aligned_addresses_spread_over_the_slots(void)
{
    static int used[TEX_MEMO_SLOTS];
    memset(used, 0, sizeof(used));
    int distinct = 0;
    const int textures = 256;
    for (int i = 0; i < textures; i++) {
        Ident id;
        memset(&id, 0, sizeof(id));
        id.data = (const void*)(uintptr_t)(0x30000000u + (uint32_t)i * 0x800u);
        id.width = id.height = 32;
        uint32_t slot = tex_memo_hash(&id, sizeof(id)) & (TEX_MEMO_SLOTS - 1);
        distinct += used[slot]++ == 0;
    }
    CHECK(distinct >= textures * 3 / 4); /* a fair hash keeps ~78% distinct at this load */
}

int main(void)
{
    test_empty_table_finds_nothing();
    test_finds_what_this_frame_stored();
    test_forgets_at_the_next_frame();
    test_frame_zero_is_never_stored();
    test_a_slot_keeps_the_last_identity();
    test_negative_entries_are_not_stored();
    test_hash_is_a_function_of_the_bytes();
    test_hash_reads_a_trailing_partial_word();
    test_aligned_addresses_spread_over_the_slots();
    if (failures != 0) {
        fprintf(stderr, "tex_memo: %d check(s) failed\n", failures);
        return 1;
    }
    printf("tex_memo: all tests passed\n");
    return 0;
}
