#ifndef AC3DS_GX_DLIST_H
#define AC3DS_GX_DLIST_H

#include <stdbool.h>
#include <stdint.h>

#define GX_DL_MAX_ARGS 6

typedef struct {
    uint8_t* buf;
    uint32_t size;
    uint32_t used;
    bool active;
    bool overflow;
} GxDlRecorder;

typedef void (*GxDlHandler)(uint32_t op, const uint32_t* args, uint32_t count, void* ctx);

GxDlRecorder gx_dl_begin(void* buf, uint32_t size);
bool gx_dl_recording(const GxDlRecorder* r);
/* False if it doesn't fit or count > GX_DL_MAX_ARGS, which spoils the whole list. */
bool gx_dl_record(GxDlRecorder* r, uint32_t op, const uint32_t* args, uint32_t count);
/* Bytes recorded, 0 after an overflow. */
uint32_t gx_dl_end(GxDlRecorder* r);
uint32_t gx_dl_replay(const void* list, uint32_t nbytes, GxDlHandler handler, void* ctx);

#endif /* AC3DS_GX_DLIST_H */
