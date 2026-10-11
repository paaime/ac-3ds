/* Host tests for disc_check.c. */
#include <stdio.h>
#include <string.h>

#include "../disc_check.h"

static int failures;
#define CHECK(c)                                                                  \
    do {                                                                          \
        if (!(c)) {                                                               \
            fprintf(stderr, "%s:%d: CHECK failed: %s\n", __FILE__, __LINE__, #c); \
            failures++;                                                           \
        }                                                                         \
    } while (0)

static void test_disc_image_names(void)
{
    CHECK(disc_check_name("Animal Crossing (USA).iso"));
    CHECK(disc_check_name("ac.GCM"));
    CHECK(disc_check_name("ac.ciso"));
    CHECK(!disc_check_name("._Animal Crossing (USA).iso")); /* the copy's macOS metadata */
    CHECK(!disc_check_name(".hidden.iso"));
    CHECK(!disc_check_name("ac.iso.txt"));
    CHECK(!disc_check_name("save"));
    CHECK(!disc_check_name(".iso"));
    CHECK(!disc_check_name(""));
    CHECK(!disc_check_name(NULL));
}

/* The disc header's first 8 bytes: game ID (6), disc number, revision. */
static void test_the_usa_first_release(void)
{
    const unsigned char ok[8] = { 'G', 'A', 'F', 'E', '0', '1', 0, 0 };
    CHECK(disc_check_id(ok) == DISC_ID_OK);
}

static void test_other_regions(void)
{
    const unsigned char eu[8] = { 'G', 'A', 'F', 'P', '0', '1', 0, 0 };
    const unsigned char jp[8] = { 'G', 'A', 'F', 'J', '0', '1', 0, 0 };
    CHECK(disc_check_id(eu) == DISC_ID_OTHER_REGION);
    CHECK(disc_check_id(jp) == DISC_ID_OTHER_REGION);
    CHECK(strcmp(disc_check_region('P'), "European") == 0);
    CHECK(strcmp(disc_check_region('J'), "Japanese") == 0);
    CHECK(strcmp(disc_check_region('U'), "Australian") == 0);
    CHECK(strcmp(disc_check_region('E'), "USA") == 0);
    CHECK(disc_check_region('X') == NULL); /* the message then says "another region" */
    const unsigned char eplus[8] = { 'G', 'A', 'E', 'J', '0', '1', 0, 0 }; /* Doubutsu no Mori e+ */
    CHECK(disc_check_id(eplus) == DISC_ID_OTHER_REGION);
}

static void test_other_revisions_and_games(void)
{
    const unsigned char rev1[8] = { 'G', 'A', 'F', 'E', '0', '1', 0, 1 };
    const unsigned char other[8] = { 'G', 'M', 'S', 'E', '0', '1', 0, 0 };
    CHECK(disc_check_id(rev1) == DISC_ID_OTHER_REVISION);
    CHECK(disc_check_id(other) == DISC_ID_OTHER_GAME);
    CHECK(disc_check_id(NULL) == DISC_ID_OTHER_GAME);
}

static void test_printable_id(void)
{
    char id[DISC_ID_TEXT];
    const unsigned char odd[8] = { 'G', 'A', 0x01, 'E', 0xFF, '1', 0, 0 };
    disc_check_id_text(odd, id);
    CHECK(strcmp(id, "GA?E?1") == 0);
}

/* An .iso/.gcm holds the whole disc: it must reach the end of the file
 * table (its offset and size at 0x424 and 0x428), or the image was cut. */
static void test_a_cut_image(void)
{
    unsigned char h[DISC_HEADER_BYTES] = { 0 };
    h[0x424] = 0x00; h[0x425] = 0x0F; h[0x426] = 0xE5; h[0x427] = 0x00; /* 0xFE500 */
    h[0x428] = 0x00; h[0x429] = 0x00; h[0x42A] = 0x80; h[0x42B] = 0x00; /* 0x8000 */
    CHECK(disc_check_complete(h, 0x57058000ull));
    CHECK(disc_check_complete(h, 0xFE500 + 0x8000));
    CHECK(!disc_check_complete(h, 0xFE500 + 0x7FFF));
    CHECK(!disc_check_complete(h, 1000000));
    CHECK(!disc_check_complete(NULL, 0x57058000ull));
}

int main(void)
{
    test_disc_image_names();
    test_the_usa_first_release();
    test_other_regions();
    test_other_revisions_and_games();
    test_printable_id();
    test_a_cut_image();
    if (failures != 0) {
        fprintf(stderr, "disc_check: %d check(s) failed\n", failures);
        return 1;
    }
    printf("disc_check: all tests passed\n");
    return 0;
}
