/* backlight_3ds.c - the bottom screen's light (see backlight_3ds.h). The APT
 * hooks run on the game thread. */
#include <3ds.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

#include "backlight_3ds.h"
#include "light_timer.h"
#include "settings_3ds.h"
#include "clock_3ds.h"

#define TIMEOUT_MS (LIGHT_SAVER_TIMEOUT_S * 1000u)

static struct {
    bool ready;   /* gsp::Lcd works: off as soon as a call fails */
    bool lit;     /* as last set */
    bool started; /* the wait starts at the first scan: boot outlasts it */
    LightTimer timer;
    aptHookCookie hook;
} L;

/* gsp::Lcd is opened per switch, never held: held, it freezes the HOME Menu. */
static Result switch_light(bool lit)
{
    Result r = gspLcdInit();
    if (R_SUCCEEDED(r)) {
        r = lit ? GSPLCD_PowerOnBacklight(GSPLCD_SCREEN_BOTTOM) : GSPLCD_PowerOffBacklight(GSPLCD_SCREEN_BOTTOM);
        gspLcdExit();
    }
    return r;
}

static void set_light(bool lit)
{
    if (!L.ready || lit == L.lit) {
        return;
    }
    Result r = switch_light(lit);
    if (R_FAILED(r)) {
        fprintf(stderr, "[light] the bottom screen's light didn't switch (0x%08lX): light saver off\n",
                (unsigned long)r);
        L.ready = false;
        return;
    }
    L.lit = lit;
}

static void on_apt(APT_HookType type, void* param)
{
    (void)param;
    switch (type) {
        case APTHOOK_ONSUSPEND: /* the HOME Menu gets a lit screen */
        case APTHOOK_ONSLEEP:
        case APTHOOK_ONRESTORE:
        case APTHOOK_ONWAKEUP:
            set_light(true);
            L.timer = light_timer_start(clock3ds_ms()); /* a fresh wait on coming back */
            break;
        case APTHOOK_ONEXIT:
            set_light(true);
            break;
        default:
            break;
    }
}

static void on_abort(int signal_number)
{
    if (L.ready && !L.lit) {
        switch_light(true);
    }
    (void)signal_number; /* abort() goes on to end the program */
}

void backlight3ds_init(void)
{
    L.lit = true;
    aptHook(&L.hook, on_apt, NULL);
    atexit(backlight3ds_exit);
    signal(SIGABRT, on_abort);
    L.ready = true;
}

void backlight3ds_exit(void)
{
    if (!L.ready) {
        return;
    }
    set_light(true);
    aptUnhook(&L.hook);
    L.ready = false;
}

bool backlight3ds_scan(bool new_touch, bool held_touch)
{
    if (!L.ready) {
        return false;
    }
    if (!L.started) {
        L.started = true;
        L.timer = light_timer_start(clock3ds_ms());
    }
    bool was_lit = L.timer.lit;
    L.timer = light_timer_step(L.timer, settings3ds_on(SETTING_BOTTOM_LIGHT_SAVER), new_touch || held_touch, clock3ds_ms(),
                               TIMEOUT_MS);
    set_light(L.timer.lit);
    return new_touch && !was_lit;
}
