#ifndef AC3DS_BOTTOM_UI_3DS_H
#define AC3DS_BOTTOM_UI_3DS_H

/* The bottom screen's picture (bottom_ui_3ds.c). Plain C types: bottom_3ds.c
 * doesn't include the game's headers. */

#include <stdbool.h>

#include "bottom_layout.h"
#include "settings_file.h"

typedef enum {
    BOTTOM_PAGE_MAP,
    BOTTOM_PAGE_OPTIONS,
} BottomPage;

typedef struct {
    BottomPage page;
    Settings settings;
    SettingsPage options_page;
    int options_sheet; /* which BOTTOM_LAYOUT_OPTION_ROWS of the page's settings show */
    int acre_col, acre_row; /* the map's cursor: a tapped acre, or -1 for the player's */
    bool save_available;
} BottomUiModel;

typedef enum {
    BOTTOM_UI_DRAWN,
    BOTTOM_UI_PARTIAL, /* short of draws or textures: worth another try */
    BOTTOM_UI_NOT_YET, /* no game to draw with yet (boot): again next frame */
    BOTTOM_UI_NONE,    /* nothing could be drawn: wait for a change */
} BottomUiResult;

/* Once, before the first draw. */
void bottom_ui_init(void);

/* With the bottom render target current and the GX backend pointed at it. */
BottomUiResult bottom_ui_render(BottomUiModel model);

#endif /* AC3DS_BOTTOM_UI_3DS_H */
