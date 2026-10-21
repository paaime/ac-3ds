#ifndef AC3DS_MESSAGES_3DS_H
#define AC3DS_MESSAGES_3DS_H

#include "m_actor.h"
#include "m_choice.h"

/* Stands in for mChoice_Load_ChoseStringFromRom in m_msg.c (a rename in CMakeLists.txt). */
void ac3ds_choice_load_string(mChoice_c* choice, u8* str, int str_no, ACTOR* actor);

#endif /* AC3DS_MESSAGES_3DS_H */
