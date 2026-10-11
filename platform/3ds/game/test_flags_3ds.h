#ifndef AC3DS_TEST_FLAGS_3DS_H
#define AC3DS_TEST_FLAGS_3DS_H

/* The developer build's NES test hook (test_flags_3ds.c): with a nes_test file in
 * the data folder, SELECT + START in town starts the NES game it numbers (1 if empty). */

#include <stdbool.h>

/* Call once the data folder is the cwd. */
void test_flags_apply(void);
/* From pad3ds_hotkeys, once per input scan. */
void test_flags_pad(bool select_held, bool start_pressed);
/* Between frames (VIWaitForRetrace): carries out a pending request. */
void test_flags_frame(void);

#endif /* AC3DS_TEST_FLAGS_3DS_H */
