#ifndef AC3DS_BOTTOM_LAYOUT_H
#define AC3DS_BOTTOM_LAYOUT_H

/* PxRect: pixels, top-left origin, y down. UnitBox: menu units, centred, y up (mFont_CulcOrthoMatrix). */

#include <stdbool.h>

#define BOTTOM_SCREEN_W 320
#define BOTTOM_SCREEN_H 240
#define BOTTOM_ICON_SIZE 28
#define BOTTOM_LAYOUT_OPTION_ROWS 4

typedef struct {
    float x, y, w, h;
} PxRect;

typedef struct {
    float left, right, bottom, top;
} UnitBox;

typedef enum {
    BOTTOM_BUTTON_NONE = -1,
    BOTTOM_BUTTON_SAVE,
    BOTTOM_BUTTON_OPTIONS,
    BOTTOM_BUTTON_COUNT
} BottomButton;

PxRect bottom_layout_button(BottomButton b);
PxRect bottom_layout_button_icon(BottomButton b);
PxRect bottom_layout_button_label(BottomButton b);
PxRect bottom_layout_button_highlight(BottomButton b);
BottomButton bottom_layout_button_at(int x, int y);

PxRect bottom_layout_grid(void);
PxRect bottom_layout_panel(void);
bool bottom_layout_acre_at(UnitBox view, int x, int y, int cols, int rows, int* col, int* row);

PxRect bottom_layout_title(void);
PxRect bottom_layout_option_row(int row, bool paged);
int bottom_layout_option_at(int x, int y, int rows, bool paged);
int bottom_layout_option_sheets(int settings);

typedef enum {
    BOTTOM_PAGER_NONE,
    BOTTOM_PAGER_UP,
    BOTTOM_PAGER_DOWN,
} BottomPager;

PxRect bottom_layout_pager_up(void);
PxRect bottom_layout_pager_down(void);
PxRect bottom_layout_pager_label(void);
BottomPager bottom_layout_pager_at(int x, int y);
PxRect bottom_layout_page_tab(int tab, int count);
int bottom_layout_page_tab_at(int x, int y, int count);

/* The menu-unit view showing `src` centred in `dst` at one scale; no area gives the menus' 1:1 view. */
UnitBox bottom_layout_fit(UnitBox src, PxRect dst, float screen_w, float screen_h);

#endif /* AC3DS_BOTTOM_LAYOUT_H */
