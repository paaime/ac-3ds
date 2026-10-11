/* weeds_3ds.c - "No weeds" (see weeds_3ds.h). */
#include "pc_platform.h"

#include "m_common_data.h"

#include "settings_3ds.h"
#include "weeds_3ds.h"

static bool s_was_on;

void weeds3ds_frame(void)
{
    bool on = settings3ds_on(SETTING_NO_WEEDS);
    if (on) {
        Save_Set(clear_grass, TRUE);
    } else if (s_was_on) {
        Save_Set(clear_grass, FALSE); /* turned off: no last clearing at the next load */
    }
    s_was_on = on;
}
