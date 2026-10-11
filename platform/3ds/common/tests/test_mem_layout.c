/* Host tests for mem_layout.c. */
#include "mem_layout.h"

#include <stdio.h>

static int failures = 0;

#define CHECK(cond)                                                         \
    do {                                                                    \
        if (!(cond)) {                                                      \
            fprintf(stderr, "%s:%d: CHECK failed: %s\n", __FILE__, __LINE__, \
                    #cond);                                                 \
            failures++;                                                     \
        }                                                                   \
    } while (0)

#define MB (1024u * 1024u)
#define PAGE 0x1000u

#define MIN_POOL (8 * MB)

static void test_splits_available_memory(void)
{
    MemLayout l = mem_layout_plan(98 * MB, 24 * MB, MIN_POOL, 32 * MB);
    CHECK(l.ok);
    CHECK(l.pool_bytes == 24 * MB);
    CHECK(l.malloc_bytes == 74 * MB);
    CHECK(l.total_bytes == 98 * MB);
}

static void test_rounds_to_pages(void)
{
    MemLayout l = mem_layout_plan(98 * MB + 123, 24 * MB + 1, MIN_POOL, 32 * MB);
    CHECK(l.ok);
    CHECK(l.total_bytes % PAGE == 0 && l.pool_bytes % PAGE == 0 && l.malloc_bytes % PAGE == 0);
    CHECK(l.total_bytes == 98 * MB);
    CHECK(l.pool_bytes == 24 * MB + PAGE);
    CHECK(l.malloc_bytes + l.pool_bytes == l.total_bytes);
}

static void test_shrinks_the_pool_before_the_heap(void)
{
    /* Not enough for both: the malloc heap keeps its minimum, the pool gets the rest. */
    MemLayout l = mem_layout_plan(44 * MB, 24 * MB, MIN_POOL, 32 * MB);
    CHECK(l.ok);
    CHECK(l.malloc_bytes == 32 * MB);
    CHECK(l.pool_bytes == 12 * MB);
}

static void test_fails_below_the_minimum(void)
{
    MemLayout l = mem_layout_plan(30 * MB, 24 * MB, MIN_POOL, 32 * MB);
    CHECK(!l.ok);
    CHECK(l.malloc_bytes == 0 && l.pool_bytes == 0 && l.total_bytes == 0);
}

static void test_the_pool_keeps_its_minimum(void)
{
    /* 39 MB: room for the 32 MB malloc minimum, not for an 8 MB pool too. */
    MemLayout l = mem_layout_plan(39 * MB, 24 * MB, MIN_POOL, 32 * MB);
    CHECK(!l.ok);
    l = mem_layout_plan(40 * MB, 24 * MB, MIN_POOL, 32 * MB);
    CHECK(l.ok && l.pool_bytes == MIN_POOL && l.malloc_bytes == 32 * MB);
    l = mem_layout_plan(98 * MB, 4 * MB, MIN_POOL, 32 * MB); /* asking for less than the minimum */
    CHECK(l.ok && l.pool_bytes == MIN_POOL);
}

/* The game's usual minimums: 72 MB heap, 8 MB pool; tight: a 64 MB heap. */
static MemLayout plan_or_tight(uint32_t available, bool* tight)
{
    MemLayout l = mem_layout_plan_or_tight(available, 24 * MB, MIN_POOL, 72 * MB, 64 * MB);
    *tight = l.tight;
    return l;
}

static void test_room_enough_is_not_tight(void)
{
    bool tight = true;
    MemLayout l = plan_or_tight(103 * MB, &tight);
    CHECK(l.ok && !tight);
    CHECK(l.pool_bytes == 24 * MB && l.malloc_bytes == 79 * MB);
    l = plan_or_tight(80 * MB, &tight); /* exactly both minimums */
    CHECK(l.ok && !tight && l.malloc_bytes == 72 * MB && l.pool_bytes == MIN_POOL);
}

static void test_tight_gives_the_heap_all_but_the_pool_minimum(void)
{
    bool tight = false;
    MemLayout l = plan_or_tight(75 * MB, &tight); /* an original 3DS, the .cia's 96 MB mode */
    CHECK(l.ok && tight);
    CHECK(l.pool_bytes == MIN_POOL && l.malloc_bytes == 67 * MB);
    l = plan_or_tight(72 * MB, &tight); /* exactly the tight heap and the pool minimum */
    CHECK(l.ok && tight && l.malloc_bytes == 64 * MB && l.pool_bytes == MIN_POOL);
}

static void test_below_tight_fails(void)
{
    bool tight = false;
    MemLayout l = plan_or_tight(72 * MB - PAGE, &tight);
    CHECK(!l.ok && !tight);
    CHECK(l.total_bytes == 0 && l.malloc_bytes == 0 && l.pool_bytes == 0);
}

static void test_plain_plan_is_never_tight(void)
{
    MemLayout l = mem_layout_plan(98 * MB, 24 * MB, MIN_POOL, 32 * MB);
    CHECK(l.ok && !l.tight);
}

int main(void)
{
    test_splits_available_memory();
    test_rounds_to_pages();
    test_shrinks_the_pool_before_the_heap();
    test_fails_below_the_minimum();
    test_the_pool_keeps_its_minimum();
    test_room_enough_is_not_tight();
    test_tight_gives_the_heap_all_but_the_pool_minimum();
    test_below_tight_fails();
    test_plain_plan_is_never_tight();
    if (failures != 0) {
        fprintf(stderr, "mem_layout: %d check(s) failed\n", failures);
        return 1;
    }
    printf("mem_layout: all tests passed\n");
    return 0;
}
