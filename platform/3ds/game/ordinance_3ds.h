#ifndef AC3DS_ORDINANCE_3DS_H
#define AC3DS_ORDINANCE_3DS_H

#include <stdbool.h>

#include "m_actor_type.h"

#include "gc_msg.h"
#include "ordinance.h"

Ordinance ordinance3ds_get(void);
void ordinance3ds_set(Ordinance o);
bool ordinance3ds_sign(int index, unsigned char* buf, int size);
GcMsgChoice ordinance3ds_menu_choice(int index);
bool ordinance3ds_msg_build(int index, unsigned char* buf, int size);

/* Stand in for the game's functions through renames in CMakeLists.txt. */
u32 ac3ds_shop_item_price(mActor_name_t item_no);
void ac3ds_cockroach_decide(int player_no);

#endif /* AC3DS_ORDINANCE_3DS_H */
