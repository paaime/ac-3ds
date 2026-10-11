#ifndef AC3DS_GX_FOG_APPLY_H
#define AC3DS_GX_FOG_APPLY_H

/* A draw's GX fog on citro3d: table, colour and mode, each sent only when it changes. */

#include <stdbool.h>

#include "gx_state.h"

/* True if a new fog table was sent. */
bool gxf_apply(const GxbState* st);
/* citro3d's fog state may have changed: the next draw sends it all again. */
void gxf_invalidate(void);

#endif /* AC3DS_GX_FOG_APPLY_H */
