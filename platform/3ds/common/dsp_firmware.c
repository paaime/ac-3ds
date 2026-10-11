#include "dsp_firmware.h"

#include <string.h>

static const char MAGIC[4] = { 'D', 'S', 'P', '1' };

static uint32_t read_le32(const uint8_t* p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

uint32_t dsp_firmware_declared_size(const uint8_t* data, uint32_t bytes)
{
    if (data == NULL || bytes < DSP_FIRMWARE_HEADER_BYTES ||
        memcmp(data + DSP_FIRMWARE_MAGIC_OFFSET, MAGIC, sizeof(MAGIC)) != 0) {
        return 0;
    }
    return read_le32(data + DSP_FIRMWARE_SIZE_OFFSET);
}

bool dsp_firmware_valid(const uint8_t* data, uint32_t bytes)
{
    uint32_t declared = dsp_firmware_declared_size(data, bytes);
    return declared >= DSP_FIRMWARE_HEADER_BYTES && declared <= bytes;
}
