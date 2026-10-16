#ifndef AC3DS_TEST_FLAGS_3DS_H
#define AC3DS_TEST_FLAGS_3DS_H

#include <stdbool.h>

/* Call once the data folder is the cwd. */
void test_flags_apply(void);
void test_flags_pad(bool select_held, bool start_pressed);
void test_flags_frame(void);

#endif /* AC3DS_TEST_FLAGS_3DS_H */
