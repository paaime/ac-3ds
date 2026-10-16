#ifndef AC3DS_DSP_FIRMWARE_3DS_H
#define AC3DS_DSP_FIRMWARE_3DS_H

#include <stdint.h>

typedef enum {
    DSP_FIRMWARE_FROM_SD,
    DSP_FIRMWARE_FROM_LAUNCHER,
    DSP_FIRMWARE_NONE,
} DspFirmwareSource;

typedef struct {
    void* data; /* malloc'd; the caller frees it after ndspExit */
    uint32_t size;
    DspFirmwareSource source;
} DspFirmware;

/* The SD card's copy, else the launcher's; data is NULL if neither is valid. */
DspFirmware dsp_firmware_load(void);

#endif /* AC3DS_DSP_FIRMWARE_3DS_H */
