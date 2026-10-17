#ifndef AC3DS_AUDIO_OUT_3DS_H
#define AC3DS_AUDIO_OUT_3DS_H

#include <stdbool.h>
#include <stdint.h>

typedef enum {
    AUDIO_OUT_OK,
    AUDIO_OUT_NO_FIRMWARE,
    AUDIO_OUT_DSP_FAILED,
    AUDIO_OUT_NO_MEMORY,
} AudioOutStatus;

typedef struct {
    uint32_t chunks;
    uint32_t dropped;
    uint32_t underruns;
    uint32_t pending_frames;
} AudioOutStats;

AudioOutStatus audio_out_init(float sample_rate, float gain);
void audio_out_exit(void);
bool audio_out_is_open(void);

/* Interleaved stereo; one producer at a time (the mixer or the game thread); false if dropped. */
bool audio_out_write(const int16_t* stereo, uint32_t frames);

bool audio_out_wants_chunk(uint32_t target_frames);

void audio_out_wait(int64_t timeout_ns);
void audio_out_wake(void);

AudioOutStats audio_out_stats(void);
/* Loudest sample queued since the last call, after the gain, 0..32768. */
uint32_t audio_out_take_peak(void);
const char* audio_out_status_text(AudioOutStatus status);
const char* audio_out_firmware_source(void);

#endif /* AC3DS_AUDIO_OUT_3DS_H */
