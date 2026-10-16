#ifndef AC3DS_BOTTOM_UI_3DS_H
#define AC3DS_BOTTOM_UI_3DS_H

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
    int options_sheet;
    int acre_col, acre_row; /* a tapped acre, or -1 for the player's */
    bool save_available;
} BottomUiModel;

typedef enum {
    BOTTOM_UI_DRAWN,
    BOTTOM_UI_PARTIAL,
    BOTTOM_UI_NOT_YET,
    BOTTOM_UI_NONE,
} BottomUiResult;

void bottom_ui_init(void);

/* With the bottom render target current and the GX backend pointed at it. */
BottomUiResult bottom_ui_render(BottomUiModel model);

#endif /* AC3DS_BOTTOM_UI_3DS_H */
