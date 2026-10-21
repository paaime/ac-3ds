#include "pc_platform.h"

#include <stdio.h>
#include <string.h>

#include "m_cockroach.h"
#include "m_common_data.h"
#include "m_msg_enum.h"
#include "m_shop.h"

#include "ordinance_3ds.h"
#include "ordinance_talk.h"

#define SIGN_SIZE 256

/* A spare byte of the town's save that no game code reads or writes; the PC port's load and save keep it. */
#define ORDINANCE_SAVE_BYTE (Save_Get(_241A8)[0])

Ordinance ordinance3ds_get(void)
{
    return ordinance_from_save(ORDINANCE_SAVE_BYTE);
}

void ordinance3ds_set(Ordinance o)
{
    ORDINANCE_SAVE_BYTE = ordinance_to_save(o);
    fprintf(stderr, "[ordinance] now %s\n", o == ORDINANCE_NONE ? "none" : ordinance_name(o));
}

int ac3ds_ordinance_villager_sec(int now_sec)
{
    return ordinance_villager_sec(ordinance3ds_get(), now_sec);
}

int ac3ds_ordinance_shop_open_hour(int hour)
{
    return ordinance_shop_open_hour(ordinance3ds_get(), hour);
}

int ac3ds_ordinance_shop_close_hour(int hour)
{
    return ordinance_shop_close_hour(ordinance3ds_get(), hour);
}

int ac3ds_ordinance_weeds_grow(void)
{
    return ordinance_grows_weeds(ordinance3ds_get());
}

u32 ac3ds_shop_item_price(mActor_name_t item_no)
{
    return ordinance_item_price(ordinance3ds_get(), mSP_ItemNo2ItemPrice(item_no));
}

bool ordinance3ds_sign(int index, unsigned char* buf, int size)
{
    static const int k_signs[] = { MSG_SHOP_OPEN, MSG_SHOP_CLOSED_NORMAL, MSG_SHOP2_CLOSED_NORMAL, MSG_SHOP2_OPEN };
    bool sign = false;
    for (size_t i = 0; i < sizeof(k_signs) / sizeof(k_signs[0]); i++) {
        sign = sign || index == k_signs[i];
    }
    if (!sign) {
        return false;
    }
    u8 built[SIGN_SIZE];
    size_t len = ordinance_talk_sign(ordinance3ds_get(), mSP_GetShopOpenTime(), mSP_GetShopCloseTime(), built,
                                     sizeof(built));
    if (len == 0 || (int)len > size) {
        return false;
    }
    memcpy(buf, built, len);
    return true;
}

void ac3ds_cockroach_decide(int player_no)
{
    if (ordinance_brings_cockroaches(ordinance3ds_get())) {
        mCkRh_DecideNowGokiFamilyCount(player_no);
    }
}
