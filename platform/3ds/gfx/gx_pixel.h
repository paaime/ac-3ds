#ifndef AC3DS_GX_PIXEL_H
#define AC3DS_GX_PIXEL_H

#include <stdbool.h>

#include "gx_state.h"

void gxp_apply_depth(const GxbState* st);
void gxp_apply_blend(const GxbState* st);
/* false if the GX alpha compare had to be approximated. */
bool gxp_apply_alpha_test(const GxbState* st);
/* false for GX_CULL_ALL: draw nothing. */
bool gxp_apply_cull(const GxbState* st);
/* Mark after a draw applied all four; invalidate when citro3d's state may have changed elsewhere. */
void gxp_mark_applied(void);
void gxp_invalidate(void);

#endif /* AC3DS_GX_PIXEL_H */
