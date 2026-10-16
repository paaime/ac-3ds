#include "pc_platform.h"

#include <3ds.h>
#include <dolphin/pad.h>
#include <string.h>

#include "backlight_3ds.h"
#include "bottom_3ds.h"
#include "gx_debug.h"
#include "pad_3ds.h"
#include "pad_map.h"
#include "test_flags_3ds.h"

_Static_assert((int)N3DS_KEY_A == (int)KEY_A && (int)N3DS_KEY_B == (int)KEY_B && (int)N3DS_KEY_X == (int)KEY_X && (int)N3DS_KEY_Y == (int)KEY_Y &&
                   (int)N3DS_KEY_SELECT == (int)KEY_SELECT && (int)N3DS_KEY_START == (int)KEY_START && (int)N3DS_KEY_L == (int)KEY_L &&
                   (int)N3DS_KEY_R == (int)KEY_R && (int)N3DS_KEY_ZL == (int)KEY_ZL && (int)N3DS_KEY_ZR == (int)KEY_ZR &&
                   (int)N3DS_KEY_DUP == (int)KEY_DUP && (int)N3DS_KEY_DDOWN == (int)KEY_DDOWN && (int)N3DS_KEY_DLEFT == (int)KEY_DLEFT &&
                   (int)N3DS_KEY_DRIGHT == (int)KEY_DRIGHT,
               "pad_map.h key bits must match libctru");
_Static_assert((int)GC_PAD_A == (int)PAD_BUTTON_A && (int)GC_PAD_B == (int)PAD_BUTTON_B && (int)GC_PAD_X == (int)PAD_BUTTON_X &&
                   (int)GC_PAD_Y == (int)PAD_BUTTON_Y && (int)GC_PAD_START == (int)PAD_BUTTON_START && (int)GC_PAD_Z == (int)PAD_TRIGGER_Z &&
                   (int)GC_PAD_L == (int)PAD_TRIGGER_L && (int)GC_PAD_R == (int)PAD_TRIGGER_R && (int)GC_PAD_UP == (int)PAD_BUTTON_UP &&
                   (int)GC_PAD_DOWN == (int)PAD_BUTTON_DOWN && (int)GC_PAD_LEFT == (int)PAD_BUTTON_LEFT &&
                   (int)GC_PAD_RIGHT == (int)PAD_BUTTON_RIGHT,
               "pad_map.h pad bits must match dolphin/pad.h");

#define GC_PAD_TYPE_STANDARD 0x09000000u /* SI_GC_CONTROLLER, as pc_pad.c */

static void touch_input(u32 down)
{
    bool touched = (down & KEY_TOUCH) != 0;
    bool holding = (hidKeysHeld() & KEY_TOUCH) != 0;
    bool woke_light = backlight3ds_scan(touched, holding);
    if (touched && !woke_light) {
        touchPosition t;
        hidTouchRead(&t);
        bottom3ds_touch(t.px, t.py);
    }
    bottom3ds_touch_held(holding);
}

static N3dsInput read_3ds_input(void)
{
    circlePosition pad;
    circlePosition cstick;
    hidCircleRead(&pad);
    hidCstickRead(&cstick);
    u32 held = hidKeysHeld();
    if (AC3DS_DEBUG && (held & KEY_SELECT)) { /* SELECT + L/R/START are hotkeys */
        held &= ~(u32)(KEY_L | KEY_R | KEY_START);
    }
    N3dsInput in = { held, pad.dx, pad.dy, cstick.dx, cstick.dy };
    return in;
}

void pad3ds_hotkeys(void)
{
    u32 down = hidKeysDown();
    bool select = (hidKeysHeld() & KEY_SELECT) != 0;
    bottom3ds_input();
    touch_input(down);
    if (!AC3DS_DEBUG) {
        return;
    }
    if (select && (down & KEY_L)) {
        gxd_arm();
    }
    if (select && (down & KEY_R)) {
        bottom3ds_toggle();
    }
    test_flags_pad(select, (down & KEY_START) != 0);
}

BOOL PADInit(void)
{
    return TRUE;
}

u32 PADRead(PADStatus* status)
{
    /* Ports 2-4 read as connected and idle, as in pc_pad.c. */
    memset(status, 0, sizeof(PADStatus) * PAD_MAX_CONTROLLERS);

    N3dsInput in = read_3ds_input();
    GcPadState s = pad_map(&in);
    status[0].button = s.button;
    status[0].stickX = s.stick_x;
    status[0].stickY = s.stick_y;
    status[0].substickX = s.substick_x;
    status[0].substickY = s.substick_y;
    status[0].triggerLeft = s.trigger_l;
    status[0].triggerRight = s.trigger_r;
    status[0].err = PAD_ERR_NONE;
    return PAD_CHAN0_BIT;
}

void PADControlMotor(s32 chan, u32 command)
{
    (void)chan;
    (void)command;
}

void PADControlAllMotors(const u32* commands)
{
    (void)commands;
}

void PADCleanup(void) {}

BOOL PADReset(u32 mask)
{
    (void)mask;
    return TRUE;
}

BOOL PADRecalibrate(u32 mask)
{
    (void)mask;
    return TRUE;
}

BOOL PADSync(void)
{
    return TRUE;
}

void PADSetSpec(u32 spec)
{
    (void)spec;
}

void PADSetAnalogMode(u32 mode)
{
    (void)mode;
}

BOOL PADGetType(s32 chan, u32* type)
{
    if (type != NULL) {
        *type = GC_PAD_TYPE_STANDARD;
    }
    return chan == 0;
}
