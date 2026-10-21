#ifndef AC3DS_LOOK_3DS_H
#define AC3DS_LOOK_3DS_H

#include <stdbool.h>

#include "gc_msg.h"

GcMsgChoice look3ds_menu_choice(int index);
bool look3ds_msg_build(int index, unsigned char* buf, int size);
/* Rover's last lines jump to the look talk before the arrival, which the talk ends with. */
bool look3ds_patch_rover(int index, unsigned char* buf, int len, int size);
void look3ds_frame(void);

#endif /* AC3DS_LOOK_3DS_H */
