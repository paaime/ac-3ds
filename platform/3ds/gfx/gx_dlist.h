#ifndef AC3DS_GX_DLIST_H
#define AC3DS_GX_DLIST_H

/* The GX display-list recorder, as pc_gx.c does it: the few GX calls the game
 * records (m_play's EFB copy) are stored in the caller's buffer and replayed
 * through a handler. A magic word marks the lists this recorder wrote. */

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
/* False (and the list is spoiled) if it doesn't fit or count > GX_DL_MAX_ARGS. */
bool gx_dl_record(GxDlRecorder* r, uint32_t op, const uint32_t* args, uint32_t count);
/* Ends recording: the list's size in bytes, or 0 if it overflowed. */
uint32_t gx_dl_end(GxDlRecorder* r);
/* Replays a recorded list; returns the number of commands handed to `handler`. */
uint32_t gx_dl_replay(const void* list, uint32_t nbytes, GxDlHandler handler, void* ctx);

#endif /* AC3DS_GX_DLIST_H */
