#include "pc_platform.h"
#include "pc_settings.h"

#include "m_event.h"

#include "settings_3ds.h"
#include "shop_3ds.h"

void shop3ds_frame(void)
{
    bool wanted = settings3ds_on(SETTING_SHOP_WITHOUT_VISITOR);
    bool pending = mEv_CheckEvent(mEv_SAVED_RENEWSHOP) == TRUE;
    bool was_on = g_pc_settings.disable_shop_visitor_req != 0;
    /* Turned off during a pending renovation, the shop would stay closed (mSP_GetRealShopLevel). */
    g_pc_settings.disable_shop_visitor_req = wanted || (pending && was_on);
}
