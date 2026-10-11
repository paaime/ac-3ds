#ifndef AC3DS_AUDIO_OUT_3DS_H
#define AC3DS_AUDIO_OUT_3DS_H

/* Sound output through ndsp (audio_out_3ds.c): one stereo PCM16 channel.
 * audio_out_write has one producer at a time (the mixer or the game thread). */

#include <stdbool.h>
#include <stdint.h>

typedef enum {
    AUDIO_OUT_OK,
    AUDIO_OUT_NO_FIRMWARE, /* no sdmc:/3ds/dspfirm.cdc and no launcher copy */
    AUDIO_OUT_DSP_FAILED,  /* ndspInit failed with the firmware loaded */
    AUDIO_OUT_NO_MEMORY,
} AudioOutStatus;

typedef struct {
    uint32_t chunks;         /* chunks queued since start */
    uint32_t dropped;        /* chunks dropped because every buffer was busy */
    uint32_t underruns;      /* times the queue had run dry when a chunk arrived */
    uint32_t pending_frames; /* frames waiting to play now */
} AudioOutStats;

AudioOutStatus audio_out_init(float sample_rate, float volume);
void audio_out_exit(void);
bool audio_out_is_open(void);

/* Queues `frames` interleaved stereo frames; false if dropped. */
bool audio_out_write(const int16_t* stereo, uint32_t frames);

/* True while less than `target_frames` is waiting to play and a buffer is free. */
bool audio_out_wants_chunk(uint32_t target_frames);

/* Sleeps until ndsp finishes its next DSP frame (about every 5 ms), at most `timeout_ns`. */
void audio_out_wait(int64_t timeout_ns);
/* Ends a pending audio_out_wait early. */
void audio_out_wake(void);

AudioOutStats audio_out_stats(void);
/* The loudest sample queued since the last call (0..32768), then resets. */
uint32_t audio_out_take_peak(void);
const char* audio_out_status_text(AudioOutStatus status);
/* Where the DSP firmware came from ("SD card", "launcher"), or NULL if closed. */
const char* audio_out_firmware_source(void);

#endif /* AC3DS_AUDIO_OUT_3DS_H */
