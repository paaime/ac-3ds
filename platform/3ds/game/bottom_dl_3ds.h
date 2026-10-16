#ifndef AC3DS_BOTTOM_DL_3DS_H
#define AC3DS_BOTTOM_DL_3DS_H

struct game_s;

typedef enum {
    BOTTOM_DL_DRAWN,
    BOTTOM_DL_PARTIAL,
    BOTTOM_DL_NO_GAME,
    BOTTOM_DL_NONE,
} BottomDlResult;

/* Writes into game->graph's polygon list, in the menus' screen units. */
typedef void (*BottomDlBuild)(struct game_s* game, const void* context);

/* At the end of a frame, after the frame's own emu64 pass. */
BottomDlResult bottom_dl_render(BottomDlBuild build, const void* context);

#endif /* AC3DS_BOTTOM_DL_3DS_H */
