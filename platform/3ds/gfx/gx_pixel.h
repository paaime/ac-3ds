#ifndef AC3DS_GX_PIXEL_H
#define AC3DS_GX_PIXEL_H

/* GX pixel pipeline state -> citro3d: depth, blending, alpha test, culling. */

#include <stdbool.h>

#include "gx_state.h"

void gxp_apply_depth(const GxbState* st);
void gxp_apply_blend(const GxbState* st);
/* Returns false if the GX alpha compare had to be approximated. */
bool gxp_apply_alpha_test(const GxbState* st);
/* Returns false if nothing is drawn (GX_CULL_ALL). */
bool gxp_apply_cull(const GxbState* st);
/* The apply_* calls skip unchanged state: mark after a draw applied all four;
 * invalidate when something else may have changed citro3d's state. */
void gxp_mark_applied(void);
void gxp_invalidate(void);

#endif /* AC3DS_GX_PIXEL_H */
