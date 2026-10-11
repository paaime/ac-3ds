#ifndef AC3DS_WEEDS_3DS_H
#define AC3DS_WEEDS_3DS_H

/* "No weeds" (weeds_3ds.c): keeps up the town's clear_grass flag (Wisp's wish), which each town
 * load uses to pull every weed and then lowers (mAGrw_RenewalFgItem_ovl). */

/* Between frames (VIWaitForRetrace). */
void weeds3ds_frame(void);

#endif /* AC3DS_WEEDS_3DS_H */
