/* Host tests for gx_tev_cache.c. A fake translator stands in for
 * gx_tev_translate: it counts its calls and writes its input's register 1
 * into the result, so a test can tell whose result came back. */
#include <stdio.h>
#include <string.h>

#include "../gx_tev_cache.h"

static int failures;
#define CHECK(c)                                                                  \
    do {                                                                          \
        if (!(c)) {                                                               \
            fprintf(stderr, "%s:%d: CHECK failed: %s\n", __FILE__, __LINE__, #c); \
            failures++;                                                           \
        }                                                                         \
    } while (0)

static int s_calls;
static bool s_fail;

static bool fake_translate(const GxTevConfig* gx, PicaCombiner* out, const char** reason)
{
    s_calls++;
    if (s_fail) {
        if (reason != NULL) {
            *reason = "unsupported";
        }
        return false;
    }
    memset(out, 0, sizeof(*out));
    out->stage_count = gx->stage_count;
    out->buffer_color = gx->reg[1];
    return true;
}

static GxTevCache s_cache;

static GxTevConfig config(int stages, uint32_t reg1)
{
    GxTevConfig c;
    memset(&c, 0, sizeof(c));
    c.stage_count = stages;
    for (int i = 0; i < stages; i++) {
        c.stages[i].color.in[3] = (uint8_t)(i + 1);
        c.stages[i].tex_map = (uint8_t)i;
    }
    c.reg[1] = reg1;
    return c;
}

static void reset(void)
{
    gx_tev_cache_clear(&s_cache);
    s_calls = 0;
    s_fail = false;
}

static void test_translates_once_then_remembers(void)
{
    reset();
    GxTevConfig c = config(2, 0x11223344u);
    const PicaCombiner* a = gx_tev_cache_get(&s_cache, &c, fake_translate, NULL);
    CHECK(a != NULL && a->buffer_color == 0x11223344u && a->stage_count == 2);
    const PicaCombiner* b = gx_tev_cache_get(&s_cache, &c, fake_translate, NULL);
    CHECK(b != NULL && b->buffer_color == 0x11223344u);
    CHECK(s_calls == 1);
    CHECK(s_cache.hits == 1 && s_cache.misses == 1);
}

static void test_unused_stages_dont_matter(void)
{
    reset();
    GxTevConfig c = config(1, 7);
    gx_tev_cache_get(&s_cache, &c, fake_translate, NULL);
    c.stages[5].color.in[0] = 9; /* beyond stage_count: GX ignores it */
    c.stages[GX_TEV_MAX_STAGES - 1].alpha.op = 1;
    gx_tev_cache_get(&s_cache, &c, fake_translate, NULL);
    CHECK(s_calls == 1);
}

static void test_a_used_stage_or_a_register_does(void)
{
    reset();
    GxTevConfig c = config(2, 7);
    gx_tev_cache_get(&s_cache, &c, fake_translate, NULL);
    c.stages[1].alpha.scale = 2;
    gx_tev_cache_get(&s_cache, &c, fake_translate, NULL);
    CHECK(s_calls == 2);
    c.konst[3] = 0xFF;
    gx_tev_cache_get(&s_cache, &c, fake_translate, NULL);
    CHECK(s_calls == 3);
    GxTevConfig d = config(2, 8);
    const PicaCombiner* p = gx_tev_cache_get(&s_cache, &d, fake_translate, NULL);
    CHECK(s_calls == 4 && p != NULL && p->buffer_color == 8);
}

static void test_failures_are_reported_and_not_kept(void)
{
    reset();
    s_fail = true;
    GxTevConfig c = config(3, 1);
    const char* reason = NULL;
    CHECK(gx_tev_cache_get(&s_cache, &c, fake_translate, &reason) == NULL);
    CHECK(reason != NULL && strcmp(reason, "unsupported") == 0);
    CHECK(gx_tev_cache_get(&s_cache, &c, fake_translate, NULL) == NULL);
    CHECK(s_calls == 2);
    s_fail = false;
    CHECK(gx_tev_cache_get(&s_cache, &c, fake_translate, NULL) != NULL);
    CHECK(s_calls == 3);
}

/* More setups than slots: collisions evict, but every result is still the
 * one for its own setup. */
static void test_each_setup_gets_its_own_result(void)
{
    reset();
    const int setups = GX_TEV_CACHE_SLOTS * 4;
    for (int round = 0; round < 2; round++) {
        for (int i = 0; i < setups; i++) {
            GxTevConfig c = config(1 + i % 3, (uint32_t)i * 2654435761u);
            const PicaCombiner* p = gx_tev_cache_get(&s_cache, &c, fake_translate, NULL);
            CHECK(p != NULL && p->buffer_color == (uint32_t)i * 2654435761u && p->stage_count == 1 + i % 3);
        }
    }
    CHECK(s_calls > setups); /* the second round missed some */
}

static void test_clear_forgets(void)
{
    reset();
    GxTevConfig c = config(1, 5);
    gx_tev_cache_get(&s_cache, &c, fake_translate, NULL);
    gx_tev_cache_clear(&s_cache);
    gx_tev_cache_get(&s_cache, &c, fake_translate, NULL);
    CHECK(s_calls == 2);
    CHECK(s_cache.hits == 0 && s_cache.misses == 1);
}

/* An out-of-range stage count goes to the translator as is (which rejects
 * it) and never reads past the stages. */
static void test_odd_stage_counts_reach_the_translator(void)
{
    reset();
    GxTevConfig c = config(0, 3);
    c.stage_count = GX_TEV_MAX_STAGES + 5;
    const PicaCombiner* p = gx_tev_cache_get(&s_cache, &c, fake_translate, NULL);
    CHECK(p != NULL && p->stage_count == GX_TEV_MAX_STAGES + 5);
    c.stage_count = -1;
    p = gx_tev_cache_get(&s_cache, &c, fake_translate, NULL);
    CHECK(p != NULL && p->stage_count == -1);
    CHECK(s_calls == 2);
}

int main(void)
{
    test_translates_once_then_remembers();
    test_unused_stages_dont_matter();
    test_a_used_stage_or_a_register_does();
    test_failures_are_reported_and_not_kept();
    test_each_setup_gets_its_own_result();
    test_clear_forgets();
    test_odd_stage_counts_reach_the_translator();
    if (failures != 0) {
        fprintf(stderr, "gx_tev_cache: %d check(s) failed\n", failures);
        return 1;
    }
    printf("gx_tev_cache: all tests passed\n");
    return 0;
}
