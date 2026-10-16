#ifndef AC3DS_GX_FOG_APPLY_H
#define AC3DS_GX_FOG_APPLY_H

#include <stdbool.h>

#include "gx_state.h"

/* True if a new fog table was sent. */
bool gxf_apply(const GxbState* st);
void gxf_invalidate(void);

#endif /* AC3DS_GX_FOG_APPLY_H */
