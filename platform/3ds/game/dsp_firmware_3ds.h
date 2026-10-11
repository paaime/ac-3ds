#ifndef AC3DS_DSP_FIRMWARE_3DS_H
#define AC3DS_DSP_FIRMWARE_3DS_H

/* Finds the DSP firmware component ndsp needs (dsp_firmware_3ds.c). */

#include <stdint.h>

typedef enum {
    DSP_FIRMWARE_FROM_SD,       /* sdmc:/3ds/dspfirm.cdc */
    DSP_FIRMWARE_FROM_LAUNCHER, /* the Homebrew Launcher's copy (hb:ndsp) */
    DSP_FIRMWARE_NONE,
} DspFirmwareSource;

typedef struct {
    void* data; /* malloc'd; the caller frees it after ndspExit */
    uint32_t size;
    DspFirmwareSource source;
} DspFirmware;

/* A validated copy from the SD card, else from the launcher; data is NULL if neither works. */
DspFirmware dsp_firmware_load(void);

#endif /* AC3DS_DSP_FIRMWARE_3DS_H */
