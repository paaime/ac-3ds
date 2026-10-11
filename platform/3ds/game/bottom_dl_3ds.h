#ifndef AC3DS_BOTTOM_DL_3DS_H
#define AC3DS_BOTTOM_DL_3DS_H

/* The bottom screen's own display list (bottom_dl_3ds.c): game-style drawing
 * run through emu64 into the current render target. */

struct game_s;

typedef enum {
    BOTTOM_DL_DRAWN,
    BOTTOM_DL_PARTIAL, /* draws dropped or textures missing: worth another try */
    BOTTOM_DL_NO_GAME, /* no game to draw with yet: try again */
    BOTTOM_DL_NONE,    /* the list didn't fit */
} BottomDlResult;

/* Writes the list into game->graph's polygon list, in the menus' screen units. */
typedef void (*BottomDlBuild)(struct game_s* game, const void* context);

/* At the end of a frame, after the frame's own emu64 pass. */
BottomDlResult bottom_dl_render(BottomDlBuild build, const void* context);

#endif /* AC3DS_BOTTOM_DL_3DS_H */
