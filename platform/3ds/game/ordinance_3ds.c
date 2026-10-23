#include "pc_platform.h"

#include <stdio.h>
#include <string.h>

#include "m_cockroach.h"
#include "m_common_data.h"
#include "m_land.h"
#include "m_msg.h"
#include "m_msg_enum.h"
#include "m_private.h"
#include "m_shop.h"

#include "ordinance_3ds.h"
#include "ordinance_talk.h"

#define SIGN_SIZE          256
/* The Able Sisters' door when they're closed (aNW_talk_door_demo_ct). */
#define TAILOR_CLOSED_SIGN MSG_2010

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

int ac3ds_ordinance_tailor_open(int now_sec)
{
    return ordinance_tailor_open(ordinance3ds_get(), now_sec);
}

int ac3ds_ordinance_weeds_grow(void)
{
    return ordinance_grows_weeds(ordinance3ds_get());
}

u32 ac3ds_shop_item_price(mActor_name_t item_no)
{
    return ordinance_item_price(ordinance3ds_get(), mSP_ItemNo2ItemPrice(item_no));
}

static bool shop_sign(int index)
{
    static const int k_signs[] = { MSG_SHOP_OPEN, MSG_SHOP_CLOSED_NORMAL, MSG_SHOP2_CLOSED_NORMAL, MSG_SHOP2_OPEN };
    for (size_t i = 0; i < sizeof(k_signs) / sizeof(k_signs[0]); i++) {
        if (index == k_signs[i]) {
            return true;
        }
    }
    return false;
}

bool ordinance3ds_sign(int index, unsigned char* buf, int size)
{
    Ordinance o = ordinance3ds_get();
    int open_hour;
    int close_hour;
    if (shop_sign(index)) {
        open_hour = mSP_GetShopOpenTime();
        close_hour = mSP_GetShopCloseTime();
    } else if (index == TAILOR_CLOSED_SIGN) {
        open_hour = ordinance_tailor_open_hour(o);
        close_hour = ordinance_tailor_close_hour(o);
    } else {
        return false;
    }
    u8 built[SIGN_SIZE];
    size_t len = ordinance_talk_sign(o, open_hour, close_hour, built, sizeof(built));
    if (len == 0 || (int)len > size) {
        return false;
    }
    memcpy(buf, built, len);
    return true;
}

/* Pelly's menus: aPG_set_talk_info's greetings, then aPG_repay_after_init's. Phyllis's follow each. */
static const int k_pelly_menus[] = {
    0x31d3, 0x31d1, 0x08b1, 0x08ad, 0x08d1, 0x08cf, 0x31d7, 0x31d5, 0x08db, 0x08d9, 0x08df, 0x08dd,
};

static int menu_speaker(int index)
{
    for (size_t i = 0; i < sizeof(k_pelly_menus) / sizeof(k_pelly_menus[0]); i++) {
        if (index == k_pelly_menus[i]) {
            return ORDINANCE_TALK_PELLY;
        }
        if (index == k_pelly_menus[i] + 1) {
            return ORDINANCE_TALK_PHYLLIS;
        }
    }
    return -1;
}

GcMsgChoice ordinance3ds_menu_choice(int index)
{
    GcMsgChoice none = { 0, 0 };
    int speaker = menu_speaker(index);
    if (speaker < 0 || mLd_PlayerManKindCheck() != NATIVE) {
        return none;
    }
    GcMsgChoice ordinances = { ORDINANCE_TALK_STRING_ORDINANCES,
                               (uint16_t)ordinance_talk_intro((OrdinanceTalkSpeaker)speaker) };
    return ordinances;
}

bool ordinance3ds_msg_build(int index, unsigned char* buf, int size)
{
    if (!ordinance_talk_owns(index)) {
        return false;
    }
    OrdinanceTalkState st = { ordinance3ds_get(), Now_Private != NULL ? Now_Private->inventory.wallet : 0 };
    size_t len = size > 0 ? ordinance_talk_build(index, st, buf, (size_t)size) : 0;
    if (len == 0) {
        fprintf(stderr, "[ordinance] message %d couldn't be built\n", index);
    }
    return len != 0;
}

static void enact(Ordinance o)
{
    if (Now_Private == NULL || !ordinance_affordable(Now_Private->inventory.wallet)) {
        fprintf(stderr, "[ordinance] %s not enacted: the wallet can't pay\n", ordinance_name(o));
        return;
    }
    Now_Private->inventory.wallet = ordinance_pay(Now_Private->inventory.wallet);
    ordinance3ds_set(o);
}

int ac3ds_post_ordinance_talk(void)
{
    OrdinanceTalkStep step = ordinance_talk_step(mMsg_Get_msg_num(mMsg_Get_base_window_p()));
    if (step.effect == ORDINANCE_TALK_ENACT) {
        enact(step.ordinance);
    } else if (step.effect == ORDINANCE_TALK_REPEAL) {
        ordinance3ds_set(ORDINANCE_NONE);
    }
    return step.back_to_menu;
}

void ac3ds_cockroach_decide(int player_no)
{
    if (ordinance_brings_cockroaches(ordinance3ds_get())) {
        mCkRh_DecideNowGokiFamilyCount(player_no);
    }
}
