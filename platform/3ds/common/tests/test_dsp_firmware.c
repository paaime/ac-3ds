#include "dsp_firmware.h"

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

#define IMAGE_BYTES 0x400

static void make_image(uint8_t* img, uint32_t declared)
{
    memset(img, 0, IMAGE_BYTES);
    memcpy(img + DSP_FIRMWARE_MAGIC_OFFSET, "DSP1", 4);
    img[DSP_FIRMWARE_SIZE_OFFSET + 0] = (uint8_t)(declared);
    img[DSP_FIRMWARE_SIZE_OFFSET + 1] = (uint8_t)(declared >> 8);
    img[DSP_FIRMWARE_SIZE_OFFSET + 2] = (uint8_t)(declared >> 16);
    img[DSP_FIRMWARE_SIZE_OFFSET + 3] = (uint8_t)(declared >> 24);
}

static void test_reads_the_declared_size(void)
{
    uint8_t img[IMAGE_BYTES];
    make_image(img, 0x1234);
    CHECK(dsp_firmware_declared_size(img, IMAGE_BYTES) == 0x1234);
}

static void test_valid_image(void)
{
    uint8_t img[IMAGE_BYTES];
    make_image(img, IMAGE_BYTES);
    CHECK(dsp_firmware_valid(img, IMAGE_BYTES));
}

static void test_rejects_a_missing_magic(void)
{
    uint8_t img[IMAGE_BYTES];
    make_image(img, IMAGE_BYTES);
    img[DSP_FIRMWARE_MAGIC_OFFSET] = 'X';
    CHECK(!dsp_firmware_valid(img, IMAGE_BYTES));
    CHECK(dsp_firmware_declared_size(img, IMAGE_BYTES) == 0);
}

static void test_rejects_a_size_beyond_the_data(void)
{
    uint8_t img[IMAGE_BYTES];
    make_image(img, IMAGE_BYTES + 1);
    CHECK(!dsp_firmware_valid(img, IMAGE_BYTES));
}

static void test_rejects_a_truncated_header(void)
{
    uint8_t img[IMAGE_BYTES];
    make_image(img, IMAGE_BYTES);
    CHECK(!dsp_firmware_valid(img, DSP_FIRMWARE_HEADER_BYTES - 1));
    CHECK(dsp_firmware_declared_size(img, DSP_FIRMWARE_HEADER_BYTES - 1) == 0);
    CHECK(!dsp_firmware_valid(NULL, IMAGE_BYTES));
}

static void test_rejects_a_size_smaller_than_the_header(void)
{
    uint8_t img[IMAGE_BYTES];
    make_image(img, 4);
    CHECK(!dsp_firmware_valid(img, IMAGE_BYTES));
}

int main(void)
{
    test_reads_the_declared_size();
    test_valid_image();
    test_rejects_a_missing_magic();
    test_rejects_a_size_beyond_the_data();
    test_rejects_a_truncated_header();
    test_rejects_a_size_smaller_than_the_header();
    if (failures == 0) {
        printf("dsp_firmware: all tests passed\n");
    }
    return failures == 0 ? 0 : 1;
}
