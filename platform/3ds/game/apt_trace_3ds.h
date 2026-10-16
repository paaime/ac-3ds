#ifndef AC3DS_APT_TRACE_3DS_H
#define AC3DS_APT_TRACE_3DS_H

#include <stdbool.h>

/* Before C3D_Init and ndspInit. */
void apt_trace3ds_before_libs(bool detailed);
/* After ndspInit. */
void apt_trace3ds_after_libs(void);

#endif /* AC3DS_APT_TRACE_3DS_H */
