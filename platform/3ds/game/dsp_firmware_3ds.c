#include <3ds.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "dsp_firmware.h"
#include "dsp_firmware_3ds.h"

#define FIRMWARE_PATH       "sdmc:/3ds/dspfirm.cdc"
#define LAUNCHER_SERVICE    "hb:ndsp"
#define LAUNCHER_MAP_ADDR   OS_HEAP_AREA_BEGIN /* free: there is no regular heap */
#define MAX_FIRMWARE_BYTES  (1024u * 1024u)

static const DspFirmware NO_FIRMWARE = { NULL, 0, DSP_FIRMWARE_NONE };

static long file_size(FILE* f)
{
    if (fseek(f, 0, SEEK_END) != 0) {
        return -1;
    }
    long n = ftell(f);
    return fseek(f, 0, SEEK_SET) == 0 ? n : -1;
}

static DspFirmware load_from_sd(void)
{
    FILE* f = fopen(FIRMWARE_PATH, "rb");
    if (f == NULL) {
        return NO_FIRMWARE;
    }
    long n = file_size(f);
    uint8_t* data = (n > 0 && (unsigned long)n <= MAX_FIRMWARE_BYTES) ? malloc((size_t)n) : NULL;
    bool read_ok = data != NULL && fread(data, 1, (size_t)n, f) == (size_t)n;
    fclose(f);
    if (!read_ok || !dsp_firmware_valid(data, (uint32_t)n)) {
        free(data);
        fprintf(stderr, "[audio] %s is unreadable or not a DSP firmware file\n", FIRMWARE_PATH);
        return NO_FIRMWARE;
    }
    DspFirmware fw = { data, (uint32_t)n, DSP_FIRMWARE_FROM_SD };
    return fw;
}

static uint32_t mapped_bytes(u32 addr)
{
    MemInfo info;
    PageInfo page;
    if (R_FAILED(svcQueryMemory(&info, &page, addr)) || info.base_addr > addr) {
        return 0;
    }
    return info.base_addr + info.size - addr;
}

/* Not ndspInit's own search: it maps this at fake_heap_end, inside the GPU pool here (all memory is linear). */
static DspFirmware load_from_launcher(void)
{
    Handle block = envGetHandle(LAUNCHER_SERVICE);
    if (block == 0 || R_FAILED(svcMapMemoryBlock(block, LAUNCHER_MAP_ADDR, MEMPERM_READWRITE, MEMPERM_READWRITE))) {
        return NO_FIRMWARE;
    }
    const uint8_t* mapped = (const uint8_t*)LAUNCHER_MAP_ADDR;
    uint32_t available = mapped_bytes(LAUNCHER_MAP_ADDR);
    uint32_t declared = dsp_firmware_declared_size(mapped, available);
    bool ok = declared <= MAX_FIRMWARE_BYTES && dsp_firmware_valid(mapped, available);
    uint8_t* data = ok ? malloc(declared) : NULL;
    if (data != NULL) {
        memcpy(data, mapped, declared);
    }
    svcUnmapMemoryBlock(block, LAUNCHER_MAP_ADDR);
    if (data == NULL) {
        return NO_FIRMWARE;
    }
    DspFirmware fw = { data, declared, DSP_FIRMWARE_FROM_LAUNCHER };
    return fw;
}

DspFirmware dsp_firmware_load(void)
{
    DspFirmware fw = load_from_sd();
    return fw.data != NULL ? fw : load_from_launcher();
}
