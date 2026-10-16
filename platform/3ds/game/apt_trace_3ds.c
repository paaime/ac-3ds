#include <3ds.h>
#include <stdio.h>

#include "apt_trace_3ds.h"
#include "trace_3ds.h"
#include "clock_3ds.h"

#define KB 1024u

static struct {
    bool on;
    aptHookCookie apt_after_libs, apt_before_libs;
    dspHookCookie dsp_after_libs, dsp_before_libs;
} T;

static void step(const char* stage)
{
    trace3ds_mark(stage);
    fprintf(stderr, "[apt] %llu ms: %s\n", (unsigned long long)clock3ds_ms(), stage);
}

/* libctru calls hooks newest first: registered last, this runs before citro3d's and ndsp's. */
static void apt_first(APT_HookType type, void* param)
{
    (void)param;
    switch (type) {
        case APTHOOK_ONSUSPEND:
            fprintf(stderr, "[apt] free memory: system %lu KB, base %lu KB, application %lu KB\n",
                    (unsigned long)(osGetMemRegionFree(MEMREGION_SYSTEM) / KB),
                    (unsigned long)(osGetMemRegionFree(MEMREGION_BASE) / KB),
                    (unsigned long)(osGetMemRegionFree(MEMREGION_APPLICATION) / KB));
            step("HOME: suspend hooks (citro3d waits for the GPU)");
            break;
        case APTHOOK_ONRESTORE: step("HOME: back from the HOME Menu"); break;
        case APTHOOK_ONSLEEP: step("sleep: lid closed, hooks"); break;
        case APTHOOK_ONWAKEUP: step("sleep: lid opened"); break;
        case APTHOOK_ONEXIT: step("exit: the system asked the game to close"); break;
        default: break;
    }
}

static void apt_last(APT_HookType type, void* param)
{
    (void)param;
    if (type == APTHOOK_ONSUSPEND) {
        step("HOME: suspend hooks done; VRAM save and screen hand-over to the HOME Menu (DSP sleep next)");
    } else if (type == APTHOOK_ONSLEEP) {
        step("sleep: hooks done, waiting for the lid to open");
    }
}

static void dsp_first(DSP_HookType type)
{
    if (type == DSPHOOK_ONSLEEP) {
        step("HOME/sleep: DSP going to sleep (ndsp finalising)");
    }
}

static void dsp_last(DSP_HookType type)
{
    switch (type) {
        case DSPHOOK_ONSLEEP:
            step("HOME/sleep: DSP asleep; GPU release, jump to the HOME Menu, waiting for it");
            break;
        case DSPHOOK_ONWAKEUP: step("HOME/sleep: DSP awake again (ndsp restarted)"); break;
        case DSPHOOK_ONCANCEL: step("HOME/sleep: DSP sleep cancelled"); break;
        default: break;
    }
}

void apt_trace3ds_before_libs(bool detailed)
{
    T.on = detailed;
    if (!T.on) {
        return;
    }
    aptHook(&T.apt_before_libs, apt_last, NULL);
    dspHook(&T.dsp_before_libs, dsp_last);
    fprintf(stderr, "[apt] HOME, sleep and exit steps will be logged\n");
}

void apt_trace3ds_after_libs(void)
{
    if (!T.on) {
        return;
    }
    aptHook(&T.apt_after_libs, apt_first, NULL);
    dspHook(&T.dsp_after_libs, dsp_first);
}
