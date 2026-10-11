/* audio_thread_3ds.c - the mixer thread, woken by every
 * ndsp DSP frame. It talks to the game thread through jaudio's message queues,
 * which take a lock on TARGET_PC (jaudio_NES/internal/os.c). */
#include <3ds.h>

#include "audio_out_3ds.h"
#include "audio_thread_3ds.h"

#define STACK_BYTES      (256u * 1024u) /* jaudio's mixer frames are large */
#define MIXER_CORE       2              /* New 3DS: free for applications */
#define WAIT_TIMEOUT_NS  (20LL * 1000 * 1000) /* stay responsive while ndsp is paused (HOME menu) */
#define HIGHEST_APP_PRIO 0x18
#define MAIN_PRIO_GUESS  0x30 /* libctru's main thread priority, if the query fails */
#define JOIN_TIMEOUT_NS  U64_MAX

static Thread s_thread;
static volatile bool s_running;
static AudioFillFn s_fill;
static int s_core = -1;

static void thread_main(void* arg)
{
    (void)arg;
    while (s_running) {
        s_fill();
        audio_out_wait(WAIT_TIMEOUT_NS);
    }
}

/* One step above the game thread, against system threads on core 2. */
static s32 mixer_priority(void)
{
    s32 prio = MAIN_PRIO_GUESS;
    svcGetThreadPriority(&prio, CUR_THREAD_HANDLE);
    return prio - 1 < HIGHEST_APP_PRIO ? HIGHEST_APP_PRIO : prio - 1;
}

bool audio_thread_start(AudioFillFn fill)
{
    if (s_thread != NULL || fill == NULL) {
        return s_thread != NULL;
    }
    s_fill = fill;
    s_running = true;
    bool n3ds = false;
    APT_CheckNew3DS(&n3ds);
    s_thread = n3ds ? threadCreate(thread_main, NULL, STACK_BYTES, mixer_priority(), MIXER_CORE, false) : NULL;
    if (s_thread == NULL) {
        s_running = false;
        return false;
    }
    s_core = MIXER_CORE;
    return true;
}

void audio_thread_stop(void)
{
    if (s_thread == NULL) {
        return;
    }
    s_running = false;
    audio_out_wake();
    threadJoin(s_thread, JOIN_TIMEOUT_NS);
    threadFree(s_thread);
    s_thread = NULL;
    s_core = -1;
}

int audio_thread_core(void)
{
    return s_core;
}
