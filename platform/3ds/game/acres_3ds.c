/* acres_3ds.c - "Continuous camera" (see acres_3ds.h). Applied each frame, as
 * the first day's intro and the boat ride switch the game's flag themselves. */
#include "pc_platform.h"

#include "ac_birth_control.h"
#include "m_event.h"
#include "m_field_info.h"
#include "m_player_lib.h"

#include "acre_sync.h"
#include "acres_3ds.h"
#include "settings_3ds.h"

int g_ac3ds_boat_ride; /* pc_platform.h: set by ac_boat_demo_move.c_inc */
static bool s_was_on;

void acres3ds_frame(void)
{
    int wade = mFI_GetPlayerWade();
    AcreScenes scenes = { mEv_CheckFirstIntro() == TRUE, g_ac3ds_boat_ride != 0,
                          wade == mFI_WADE_START || wade == mFI_WADE_INPROGRESS };
    bool continuous = settings3ds_on(SETTING_CONTINUOUS_CAMERA);
    AcreSync s = acre_sync(continuous, g_mPlib_wade_disabled != 0, s_was_on, scenes);
    if (s.refresh) {
        aBC_RequestNearbyRefresh(); /* as pc_settings.c does when it's turned on */
    }
    if (s.write) {
        g_mPlib_wade_disabled = s.value;
    }
    s_was_on = s.value;
}
