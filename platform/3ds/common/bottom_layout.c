#include "bottom_layout.h"

#include <stddef.h>

#define EDGE          4.0f
#define BUTTON_W      56.0f
#define BUTTON_H      44.0f
#define LABEL_GAP     1.0f
#define LABEL_INSET_X 2.0f
#define GLOW_GROW     3.0f
#define CONTENT_PAD   6.0f
#define GRID_W        196.0f
#define TITLE_X       10.0f
#define TITLE_H       28.0f
#define ROW_H         36.0f
#define ROW_GAP       2.0f
#define ROW_INSET_X   8.0f
#define PAGER_W       30.0f
#define PAGER_GAP     4.0f
#define PAGE_TAB_W    96.0f
#define PAGE_TAB_H    22.0f
#define PAGE_TAB_GAP  6.0f
/* The map menu's tiles (m_map_ovl.c, mMP_set_map_dl): A-1's centre, then 22 units a tile, rows going down. */
#define MAP_TILE_FIRST_X 11.7f
#define MAP_TILE_FIRST_Y 45.7f
#define MAP_TILE_SIZE    22.0f

static PxRect rect(float x, float y, float w, float h)
{
    PxRect r = { x, y, w, h };
    return r;
}

PxRect bottom_layout_button(BottomButton b)
{
    float right = BOTTOM_SCREEN_W - EDGE - (b == BOTTOM_BUTTON_SAVE ? BUTTON_W : 0);
    return rect(right - BUTTON_W, EDGE / 2, BUTTON_W, BUTTON_H);
}

PxRect bottom_layout_button_icon(BottomButton b)
{
    PxRect r = bottom_layout_button(b);
    return rect(r.x + (r.w - BOTTOM_ICON_SIZE) / 2, r.y + GLOW_GROW, BOTTOM_ICON_SIZE, BOTTOM_ICON_SIZE);
}

PxRect bottom_layout_button_label(BottomButton b)
{
    PxRect r = bottom_layout_button(b);
    PxRect icon = bottom_layout_button_icon(b);
    float y = icon.y + icon.h + LABEL_GAP;
    return rect(r.x + LABEL_INSET_X, y, r.w - 2 * LABEL_INSET_X, r.y + r.h - y);
}

PxRect bottom_layout_button_highlight(BottomButton b)
{
    PxRect icon = bottom_layout_button_icon(b);
    return rect(icon.x - GLOW_GROW, icon.y - GLOW_GROW, icon.w + 2 * GLOW_GROW, icon.h + 2 * GLOW_GROW);
}

BottomButton bottom_layout_button_at(int x, int y)
{
    PxRect save = bottom_layout_button(BOTTOM_BUTTON_SAVE);
    PxRect options = bottom_layout_button(BOTTOM_BUTTON_OPTIONS);
    if (y < 0 || y >= options.y + options.h || x >= BOTTOM_SCREEN_W) {
        return BOTTOM_BUTTON_NONE;
    }
    if (x >= options.x) {
        return BOTTOM_BUTTON_OPTIONS;
    }
    return x >= save.x ? BOTTOM_BUTTON_SAVE : BOTTOM_BUTTON_NONE;
}

PxRect bottom_layout_grid(void)
{
    return rect(CONTENT_PAD, CONTENT_PAD, GRID_W, BOTTOM_SCREEN_H - 2 * CONTENT_PAD);
}

static PxRect under_button(void)
{
    PxRect b = bottom_layout_button(BOTTOM_BUTTON_OPTIONS);
    float y = b.y + b.h + CONTENT_PAD;
    return rect(0, y, BOTTOM_SCREEN_W, BOTTOM_SCREEN_H - y);
}

PxRect bottom_layout_panel(void)
{
    PxRect grid = bottom_layout_grid();
    PxRect below = under_button();
    float x = grid.x + grid.w + CONTENT_PAD;
    return rect(x, below.y, BOTTOM_SCREEN_W - CONTENT_PAD - x, below.h - CONTENT_PAD);
}

PxRect bottom_layout_title(void)
{
    PxRect b = bottom_layout_button(BOTTOM_BUTTON_SAVE);
    return rect(TITLE_X, b.y + (b.h - TITLE_H) / 2, b.x - CONTENT_PAD - TITLE_X, TITLE_H);
}

static float page_tabs_top(void)
{
    return BOTTOM_SCREEN_H - CONTENT_PAD - PAGE_TAB_H;
}

PxRect bottom_layout_option_row(int row, bool paged)
{
    float y = under_button().y + row * (ROW_H + ROW_GAP);
    if (row < 0 || y + ROW_H > page_tabs_top() - ROW_GAP) {
        return rect(0, 0, 0, 0);
    }
    float w = BOTTOM_SCREEN_W - 2 * ROW_INSET_X - (paged ? PAGER_W + PAGER_GAP : 0);
    return rect(ROW_INSET_X, y, w, ROW_H);
}

int bottom_layout_option_sheets(int settings)
{
    return settings <= BOTTOM_LAYOUT_OPTION_ROWS ? 1 : (settings + BOTTOM_LAYOUT_OPTION_ROWS - 1) / BOTTOM_LAYOUT_OPTION_ROWS;
}

static float pager_x(void)
{
    return BOTTOM_SCREEN_W - ROW_INSET_X - PAGER_W;
}

PxRect bottom_layout_pager_up(void)
{
    PxRect first = bottom_layout_option_row(0, true);
    return rect(pager_x(), first.y, PAGER_W, ROW_H);
}

PxRect bottom_layout_pager_down(void)
{
    PxRect last = bottom_layout_option_row(BOTTOM_LAYOUT_OPTION_ROWS - 1, true);
    return rect(pager_x(), last.y, PAGER_W, ROW_H);
}

PxRect bottom_layout_pager_label(void)
{
    PxRect up = bottom_layout_pager_up();
    PxRect down = bottom_layout_pager_down();
    float y = up.y + up.h;
    return rect(pager_x(), y, PAGER_W, down.y - y);
}

static bool hit(PxRect r, int x, int y)
{
    return x >= r.x && x < r.x + r.w && y >= r.y && y < r.y + r.h;
}

BottomPager bottom_layout_pager_at(int x, int y)
{
    if (hit(bottom_layout_pager_up(), x, y)) {
        return BOTTOM_PAGER_UP;
    }
    return hit(bottom_layout_pager_down(), x, y) ? BOTTOM_PAGER_DOWN : BOTTOM_PAGER_NONE;
}

PxRect bottom_layout_page_tab(int tab, int count)
{
    if (tab < 0 || tab >= count) {
        return rect(0, 0, 0, 0);
    }
    float total = count * PAGE_TAB_W + (count - 1) * PAGE_TAB_GAP;
    float x = (BOTTOM_SCREEN_W - total) / 2 + tab * (PAGE_TAB_W + PAGE_TAB_GAP);
    return rect(x, page_tabs_top(), PAGE_TAB_W, PAGE_TAB_H);
}

int bottom_layout_page_tab_at(int x, int y, int count)
{
    for (int tab = 0; tab < count; tab++) {
        PxRect r = bottom_layout_page_tab(tab, count);
        if (x >= r.x && x < r.x + r.w && y >= r.y && y < r.y + r.h) {
            return tab;
        }
    }
    return -1;
}

int bottom_layout_option_at(int x, int y, int rows, bool paged)
{
    for (int row = 0; row < rows; row++) {
        if (hit(bottom_layout_option_row(row, paged), x, y)) {
            return row;
        }
    }
    return -1;
}

UnitBox bottom_layout_fit(UnitBox src, PxRect dst, float screen_w, float screen_h)
{
    float src_w = src.right - src.left;
    float src_h = src.top - src.bottom;
    if (src_w <= 0 || src_h <= 0 || dst.w <= 0 || dst.h <= 0 || screen_w <= 0 || screen_h <= 0) {
        UnitBox menus = { -screen_w / 2, screen_w / 2, -screen_h / 2, screen_h / 2 };
        return menus;
    }
    float per_unit_x = dst.w / src_w;
    float per_unit_y = dst.h / src_h;
    float k = per_unit_x < per_unit_y ? per_unit_x : per_unit_y;
    float x = dst.x + (dst.w - src_w * k) / 2;
    float y = dst.y + (dst.h - src_h * k) / 2;
    UnitBox v;
    v.left = src.left - x / k;
    v.right = v.left + screen_w / k;
    v.top = src.top + y / k;
    v.bottom = v.top - screen_h / k;
    return v;
}

bool bottom_layout_acre_at(UnitBox view, int x, int y, int cols, int rows, int* col, int* row)
{
    float units_w = view.right - view.left;
    float units_h = view.top - view.bottom;
    if (col == NULL || row == NULL || units_w <= 0 || units_h <= 0) {
        return false;
    }
    float ux = view.left + (float)x * units_w / BOTTOM_SCREEN_W;
    float uy = view.top - (float)y * units_h / BOTTOM_SCREEN_H;
    float across = (ux - (MAP_TILE_FIRST_X - MAP_TILE_SIZE / 2)) / MAP_TILE_SIZE;
    float down = ((MAP_TILE_FIRST_Y + MAP_TILE_SIZE / 2) - uy) / MAP_TILE_SIZE;
    if (across < 0 || down < 0 || across >= (float)cols || down >= (float)rows) {
        return false;
    }
    *col = (int)across;
    *row = (int)down;
    return true;
}
