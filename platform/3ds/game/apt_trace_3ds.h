#ifndef AC3DS_APT_TRACE_3DS_H
#define AC3DS_APT_TRACE_3DS_H

/* With the "Detailed log" setting: the steps of a HOME, sleep or exit, logged
 * and marked for the watchdog (apt_trace_3ds.c). libctru calls hooks newest
 * first, so markers registered before citro3d and ndsp run after theirs. */

#include <stdbool.h>

/* Before C3D_Init and ndspInit. */
void apt_trace3ds_before_libs(bool detailed);
/* After ndspInit. */
void apt_trace3ds_after_libs(void);

#endif /* AC3DS_APT_TRACE_3DS_H */
