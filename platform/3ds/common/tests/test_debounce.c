#include <stdio.h>

#include "../debounce.h"

static int failures;
#define CHECK(c)                                                                  \
    do {                                                                          \
        if (!(c)) {                                                               \
            fprintf(stderr, "%s:%d: CHECK failed: %s\n", __FILE__, __LINE__, #c); \
            failures++;                                                           \
        }                                                                         \
    } while (0)

#define DELAY 2000u

static void test_nothing_pending_at_first(void)
{
    Debounce d = debounce_idle();
    CHECK(!d.pending);
    CHECK(!debounce_due(d, 0, DELAY));
    CHECK(!debounce_due(d, 100000, DELAY));
}

static void test_due_after_the_delay(void)
{
    Debounce d = debounce_change(debounce_idle(), 1000);
    CHECK(d.pending);
    CHECK(!debounce_due(d, 1000, DELAY));
    CHECK(!debounce_due(d, 2999, DELAY));
    CHECK(debounce_due(d, 3000, DELAY));
    CHECK(debounce_due(d, 50000, DELAY));
}

static void test_each_change_restarts_the_wait(void)
{
    Debounce d = debounce_change(debounce_idle(), 1000);
    d = debounce_change(d, 2500);
    CHECK(!debounce_due(d, 3000, DELAY));
    CHECK(!debounce_due(d, 4499, DELAY));
    CHECK(debounce_due(d, 4500, DELAY));
}

static void test_done_clears(void)
{
    Debounce d = debounce_done(debounce_change(debounce_idle(), 10));
    CHECK(!d.pending);
    CHECK(!debounce_due(d, 1000000, DELAY));
}

static void test_returns_copies(void)
{
    Debounce a = debounce_idle();
    Debounce b = debounce_change(a, 5);
    CHECK(!a.pending && b.pending);
    Debounce c = debounce_done(b);
    CHECK(b.pending && !c.pending);
}

static void test_a_clock_going_back_waits(void)
{
    Debounce d = debounce_change(debounce_idle(), 5000);
    CHECK(!debounce_due(d, 4000, DELAY));
}

int main(void)
{
    test_nothing_pending_at_first();
    test_due_after_the_delay();
    test_each_change_restarts_the_wait();
    test_done_clears();
    test_returns_copies();
    test_a_clock_going_back_waits();
    if (failures != 0) {
        fprintf(stderr, "debounce: %d check(s) failed\n", failures);
        return 1;
    }
    printf("debounce: all tests passed\n");
    return 0;
}
