#include <stdio.h>

#include "../look.h"

static int failures;
#define CHECK(c)                                                                  \
    do {                                                                          \
        if (!(c)) {                                                               \
            fprintf(stderr, "%s:%d: CHECK failed: %s\n", __FILE__, __LINE__, #c); \
            failures++;                                                           \
        }                                                                         \
    } while (0)

static void test_floor_round_trip(void)
{
    for (int tone = 0; tone < LOOK_TONES; tone++) {
        CHECK(look_floor_from_save(look_floor_to_save(tone)) == tone);
    }
}

static void test_no_floor_is_a_zero_byte(void)
{
    CHECK(look_floor_to_save(0) == 0);
    CHECK(look_floor_from_save(0) == 0);
}

static void test_bytes_the_port_didnt_write_have_no_floor(void)
{
    CHECK(look_floor_from_save(5) == 0);
    CHECK(look_floor_from_save(0xFF) == 0);
    CHECK(look_floor_from_save((uint8_t)(look_floor_to_save(LOOK_TONES - 1) + 1)) == 0);
    CHECK(look_floor_to_save(-1) == 0);
    CHECK(look_floor_to_save(LOOK_TONES) == look_floor_to_save(LOOK_TONES - 1));
}

static void test_faces_wrap(void)
{
    CHECK(look_face_step(0, 1) == 1);
    CHECK(look_face_step(LOOK_FACES - 1, 1) == 0);
    CHECK(look_face_step(0, -1) == LOOK_FACES - 1);
    CHECK(look_face_step(3, 0) == 3);
    CHECK(look_face_step(-5, 1) == 4);
    CHECK(look_face_step(LOOK_FACES + 2, -1) == 1);
}

static void test_tones_stop_at_the_ends(void)
{
    CHECK(look_tone_step(0, 1) == 1);
    CHECK(look_tone_step(LOOK_TONES - 1, 1) == LOOK_TONES - 1);
    CHECK(look_tone_step(0, -1) == 0);
    CHECK(look_tone_step(4, -1) == 3);
    CHECK(look_tone_step(-3, 0) == 0);
    CHECK(look_tone_step(LOOK_TONES + 3, 0) == LOOK_TONES - 1);
}

int main(void)
{
    test_floor_round_trip();
    test_no_floor_is_a_zero_byte();
    test_bytes_the_port_didnt_write_have_no_floor();
    test_faces_wrap();
    test_tones_stop_at_the_ends();
    if (failures != 0) {
        fprintf(stderr, "look: %d check(s) failed\n", failures);
        return 1;
    }
    printf("look: all tests passed\n");
    return 0;
}
