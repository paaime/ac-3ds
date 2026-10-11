/* Host tests for gx_dlist.c. */
#include "gx_dlist.h"

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

typedef struct {
    int calls;
    uint32_t ops[8];
    uint32_t first_args[8][GX_DL_MAX_ARGS];
    uint32_t counts[8];
} Seen;

static void collect(uint32_t op, const uint32_t* args, uint32_t count, void* ctx)
{
    Seen* s = ctx;
    if (s->calls < 8) {
        s->ops[s->calls] = op;
        s->counts[s->calls] = count;
        memcpy(s->first_args[s->calls], args, count * sizeof(uint32_t));
    }
    s->calls++;
}

static void test_records_and_replays_in_order(void)
{
    uint32_t buf[64];
    GxDlRecorder r = gx_dl_begin(buf, sizeof(buf));
    const uint32_t src[4] = { 0, 0, 640, 480 };
    const uint32_t copy[2] = { 0x30001000u, 0 };
    CHECK(gx_dl_recording(&r));
    CHECK(gx_dl_record(&r, 1, src, 4));
    CHECK(gx_dl_record(&r, 3, copy, 2));
    uint32_t bytes = gx_dl_end(&r);
    CHECK(bytes > 0 && !gx_dl_recording(&r));

    Seen seen = { 0 };
    CHECK(gx_dl_replay(buf, bytes, collect, &seen) == 2);
    CHECK(seen.calls == 2);
    CHECK(seen.ops[0] == 1 && seen.counts[0] == 4 && seen.first_args[0][2] == 640);
    CHECK(seen.ops[1] == 3 && seen.counts[1] == 2 && seen.first_args[1][0] == 0x30001000u);
}

static void test_overflow_returns_zero_bytes(void)
{
    uint32_t buf[4]; /* the magic word plus 3 more: too small for a 4-argument command */
    GxDlRecorder r = gx_dl_begin(buf, sizeof(buf));
    const uint32_t args[4] = { 1, 2, 3, 4 };
    gx_dl_record(&r, 1, args, 4);
    CHECK(!gx_dl_record(&r, 2, args, 4));
    CHECK(gx_dl_end(&r) == 0);
}

static void test_rejects_too_many_args(void)
{
    uint32_t buf[64];
    GxDlRecorder r = gx_dl_begin(buf, sizeof(buf));
    uint32_t args[GX_DL_MAX_ARGS + 1] = { 0 };
    CHECK(!gx_dl_record(&r, 1, args, GX_DL_MAX_ARGS + 1));
}

static void test_foreign_lists_are_not_replayed(void)
{
    /* A real GX display list (or garbage) doesn't start with the magic. */
    const uint32_t foreign[4] = { 0x98000400u, 0, 0, 0 };
    Seen seen = { 0 };
    CHECK(gx_dl_replay(foreign, sizeof(foreign), collect, &seen) == 0);
    CHECK(seen.calls == 0);
    CHECK(gx_dl_replay(NULL, 16, collect, &seen) == 0);
}

static void test_truncated_list_stops_safely(void)
{
    uint32_t buf[64];
    GxDlRecorder r = gx_dl_begin(buf, sizeof(buf));
    const uint32_t args[4] = { 1, 2, 3, 4 };
    gx_dl_record(&r, 1, args, 4);
    gx_dl_record(&r, 2, args, 4);
    uint32_t bytes = gx_dl_end(&r);
    Seen seen = { 0 };
    CHECK(gx_dl_replay(buf, bytes - 4, collect, &seen) == 1); /* the second command is cut */
}

static void test_record_without_begin_fails(void)
{
    GxDlRecorder r = { 0 };
    const uint32_t args[1] = { 7 };
    CHECK(!gx_dl_recording(&r));
    CHECK(!gx_dl_record(&r, 1, args, 1));
    CHECK(gx_dl_end(&r) == 0);
}

int main(void)
{
    test_records_and_replays_in_order();
    test_overflow_returns_zero_bytes();
    test_rejects_too_many_args();
    test_foreign_lists_are_not_replayed();
    test_truncated_list_stops_safely();
    test_record_without_begin_fails();
    if (failures == 0) {
        printf("gx_dlist: all tests passed\n");
    }
    return failures == 0 ? 0 : 1;
}
