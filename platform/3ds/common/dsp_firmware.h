#ifndef AC3DS_DSP_FIRMWARE_H
#define AC3DS_DSP_FIRMWARE_H

/* Checks a DSP firmware component (dspfirm.cdc) before the DSP gets it: a
 * 0x100-byte signature, "DSP1", then the total size (3dbrew "DSP Binary"). */

#include <stdbool.h>
#include <stdint.h>

#define DSP_FIRMWARE_MAGIC_OFFSET 0x100
#define DSP_FIRMWARE_SIZE_OFFSET  0x104
#define DSP_FIRMWARE_HEADER_BYTES 0x108

/* 0 if `bytes` can't hold a header with the magic. */
uint32_t dsp_firmware_declared_size(const uint8_t* data, uint32_t bytes);

/* The declared size must cover the header and fit in `bytes`. */
bool dsp_firmware_valid(const uint8_t* data, uint32_t bytes);

#endif /* AC3DS_DSP_FIRMWARE_H */
