#include "pc_platform.h"

#include "m_common_data.h"

#include "settings_3ds.h"
#include "weeds_3ds.h"

static bool s_was_on;

/* Each town load with clear_grass up (Wisp's wish) pulls every weed, then lowers it (mAGrw_RenewalFgItem_ovl). */
void weeds3ds_frame(void)
{
    bool on = settings3ds_on(SETTING_NO_WEEDS);
    if (on) {
        Save_Set(clear_grass, TRUE);
    } else if (s_was_on) {
        Save_Set(clear_grass, FALSE);
    }
    s_was_on = on;
}
