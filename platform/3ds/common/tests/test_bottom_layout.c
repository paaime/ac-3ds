/* Host tests for bottom_layout.c. */
#include "bottom_layout.h"

#include <math.h>
#include <stdbool.h>
#include <stdio.h>

static int failures = 0;

#define CHECK(cond)                                                         \
    do {                                                                    \
        if (!(cond)) {                                                      \
            fprintf(stderr, "%s:%d: CHECK failed: %s\n", __FILE__, __LINE__, \
                    #cond);                                                 \
            failures++;                                                     \
        }                                                                   \
    } while (0)

#define NEAR_EQ(a, b) (fabsf((a) - (b)) < 1e-3f)

static bool inside(PxRect inner, PxRect outer)
{
    return inner.x >= outer.x && inner.y >= outer.y && inner.x + inner.w <= outer.x + outer.w &&
           inner.y + inner.h <= outer.y + outer.h;
}

static bool overlap(PxRect a, PxRect b)
{
    return a.x < b.x + b.w && b.x < a.x + a.w && a.y < b.y + b.h && b.y < a.y + a.h;
}

static PxRect screen(void)
{
    PxRect s = { 0, 0, BOTTOM_SCREEN_W, BOTTOM_SCREEN_H };
    return s;
}

/* Two round buttons in the top-right corner, Save then Options: each with
 * its icon, the label under it, the disc that rings the icon when its page
 * is open. */
static void test_the_corner_buttons(void)
{
    for (int i = 0; i < BOTTOM_BUTTON_COUNT; i++) {
        BottomButton id = (BottomButton)i;
        PxRect b = bottom_layout_button(id);
        PxRect icon = bottom_layout_button_icon(id);
        PxRect label = bottom_layout_button_label(id);
        PxRect glow = bottom_layout_button_highlight(id);
        CHECK(inside(b, screen()));
        CHECK(b.y < 8 && b.x > BOTTOM_SCREEN_W / 2 && b.y + b.h < BOTTOM_SCREEN_H / 4);
        CHECK(inside(icon, b) && inside(label, b));
        CHECK(!overlap(icon, label));
        CHECK(label.y >= icon.y + icon.h);                          /* the label under the icon */
        CHECK(NEAR_EQ(icon.x + icon.w / 2, b.x + b.w / 2));         /* both centred */
        CHECK(NEAR_EQ(label.x + label.w / 2, b.x + b.w / 2));
        CHECK(NEAR_EQ(icon.w, BOTTOM_ICON_SIZE) && NEAR_EQ(icon.h, BOTTOM_ICON_SIZE));
        CHECK(inside(icon, glow) && glow.w > icon.w);               /* the highlight rings the icon */
        CHECK(NEAR_EQ(glow.x + glow.w / 2, icon.x + icon.w / 2) && NEAR_EQ(glow.y + glow.h / 2, icon.y + icon.h / 2));
        CHECK(inside(glow, screen()));
    }
    PxRect save = bottom_layout_button(BOTTOM_BUTTON_SAVE);
    PxRect options = bottom_layout_button(BOTTOM_BUTTON_OPTIONS);
    CHECK(options.x + options.w > BOTTOM_SCREEN_W - 8);            /* Options in the corner */
    CHECK(save.x + save.w <= options.x && NEAR_EQ(save.y, options.y)); /* Save just left of it */
    CHECK(NEAR_EQ(save.w, options.w) && NEAR_EQ(save.h, options.h));
    CHECK(!overlap(save, bottom_layout_grid()));
}

/* The touch areas reach the screen's top and right edges and meet between
 * the two buttons: the corner is easy to hit. */
static void test_touching_the_corner_buttons(void)
{
    PxRect save = bottom_layout_button(BOTTOM_BUTTON_SAVE);
    PxRect options = bottom_layout_button(BOTTOM_BUTTON_OPTIONS);
    CHECK(bottom_layout_button_at((int)(options.x + options.w / 2), (int)(options.y + options.h / 2)) ==
          BOTTOM_BUTTON_OPTIONS);
    CHECK(bottom_layout_button_at(BOTTOM_SCREEN_W - 1, 0) == BOTTOM_BUTTON_OPTIONS);
    CHECK(bottom_layout_button_at((int)options.x, (int)(options.y + options.h - 1)) == BOTTOM_BUTTON_OPTIONS);
    CHECK(bottom_layout_button_at((int)options.x - 1, 10) == BOTTOM_BUTTON_SAVE);
    CHECK(bottom_layout_button_at((int)(save.x + save.w / 2), (int)(save.y + save.h / 2)) == BOTTOM_BUTTON_SAVE);
    CHECK(bottom_layout_button_at((int)save.x, 0) == BOTTOM_BUTTON_SAVE);
    CHECK(bottom_layout_button_at((int)save.x - 1, 10) == BOTTOM_BUTTON_NONE);
    CHECK(bottom_layout_button_at(BOTTOM_SCREEN_W - 1, (int)(options.y + options.h)) == BOTTOM_BUTTON_NONE);
    CHECK(bottom_layout_button_at((int)save.x, (int)(save.y + save.h)) == BOTTOM_BUTTON_NONE);
    CHECK(bottom_layout_button_at(0, 0) == BOTTOM_BUTTON_NONE);
    CHECK(bottom_layout_button_at(BOTTOM_SCREEN_W, 0) == BOTTOM_BUTTON_NONE);
    CHECK(bottom_layout_button_at(BOTTOM_SCREEN_W - 1, -1) == BOTTOM_BUTTON_NONE);
    PxRect grid = bottom_layout_grid();
    CHECK(bottom_layout_button_at((int)(grid.x + grid.w - 1), (int)grid.y) == BOTTOM_BUTTON_NONE);
}

/* The map takes the whole screen: the acre grid on the left, full height;
 * the acre's panel on the right, under the button. */
static void test_the_map_fills_the_screen(void)
{
    PxRect grid = bottom_layout_grid();
    PxRect panel = bottom_layout_panel();
    PxRect b = bottom_layout_button(BOTTOM_BUTTON_OPTIONS);
    PxRect save = bottom_layout_button(BOTTOM_BUTTON_SAVE);
    CHECK(inside(grid, screen()) && inside(panel, screen()));
    CHECK(!overlap(grid, panel) && !overlap(grid, b) && !overlap(panel, b) && !overlap(panel, save));
    CHECK(grid.x < panel.x);
    CHECK(grid.h > BOTTOM_SCREEN_H * 0.9f);                     /* nearly the full height */
    CHECK(panel.y >= b.y + b.h);
    CHECK(panel.w > 90 && panel.h > 120);                       /* room for the acre's bubble */
}

static void test_option_rows(void)
{
    PxRect b = bottom_layout_button(BOTTOM_BUTTON_OPTIONS);
    PxRect save = bottom_layout_button(BOTTOM_BUTTON_SAVE);
    PxRect title = bottom_layout_title();
    CHECK(inside(title, screen()) && !overlap(title, b) && !overlap(title, save) && title.x < save.x);
    CHECK(title.w >= 98); /* the dialogue's name plate (bottom_ui_3ds.c) */
    for (int row = 0; row < BOTTOM_LAYOUT_OPTION_ROWS; row++) {
        PxRect r = bottom_layout_option_row(row, false);
        CHECK(inside(r, screen()));
        CHECK(r.y >= b.y + b.h && r.y >= title.y + title.h);    /* under the button and the title */
        CHECK(bottom_layout_option_at((int)(r.x + r.w / 2), (int)(r.y + r.h / 2), BOTTOM_LAYOUT_OPTION_ROWS, false) == row);
        if (row > 0) {
            CHECK(!overlap(r, bottom_layout_option_row(row - 1, false)));
        }
    }
    CHECK(bottom_layout_option_row(-1, false).h == 0);
    CHECK(bottom_layout_option_row(BOTTOM_LAYOUT_OPTION_ROWS - 1, false).h > 0); /* the last row that fits */
    CHECK(bottom_layout_option_row(BOTTOM_LAYOUT_OPTION_ROWS, false).h == 0);    /* one more doesn't */
    CHECK(bottom_layout_option_at(160, 10, 5, false) == -1);  /* the title */
    CHECK(bottom_layout_option_at(1, 100, 5, false) == -1);   /* left of the rows */
    PxRect last_row = bottom_layout_option_row(BOTTOM_LAYOUT_OPTION_ROWS - 1, false);
    for (int i = 0; i < 2; i++) {                      /* the page tabs, under the rows */
        PxRect tab = bottom_layout_page_tab(i, 2);
        CHECK(inside(tab, screen()));
        CHECK(tab.y >= last_row.y + last_row.h);
        CHECK(bottom_layout_page_tab_at((int)(tab.x + tab.w / 2), (int)(tab.y + tab.h / 2), 2) == i);
        CHECK(bottom_layout_option_at((int)(tab.x + tab.w / 2), (int)(tab.y + tab.h / 2), 5, false) == -1);
    }
    CHECK(!overlap(bottom_layout_page_tab(0, 2), bottom_layout_page_tab(1, 2)));
    CHECK(bottom_layout_page_tab(0, 2).x < bottom_layout_page_tab(1, 2).x);
    CHECK(NEAR_EQ(bottom_layout_page_tab(0, 2).x + bottom_layout_page_tab(1, 2).x + bottom_layout_page_tab(1, 2).w,
                  BOTTOM_SCREEN_W)); /* centred */
    CHECK(bottom_layout_page_tab(2, 2).w == 0 && bottom_layout_page_tab(-1, 2).w == 0);
    CHECK(bottom_layout_page_tab_at(160, 100, 2) == -1);
    PxRect last = bottom_layout_option_row(1, false);
    CHECK(bottom_layout_option_at(160, (int)(last.y + 2), 1, false) == -1); /* past the last row */
}

/* More settings than rows: the rows narrow for a column on their right with
 * an up arrow (level with the first row), a down arrow (level with the last)
 * and the sheet number between them. */
static void test_paged_rows_and_the_pager(void)
{
    PxRect up = bottom_layout_pager_up();
    PxRect down = bottom_layout_pager_down();
    PxRect label = bottom_layout_pager_label();
    PxRect first = bottom_layout_option_row(0, true);
    PxRect last = bottom_layout_option_row(BOTTOM_LAYOUT_OPTION_ROWS - 1, true);
    CHECK(inside(up, screen()) && inside(down, screen()) && inside(label, screen()));
    CHECK(up.x >= first.x + first.w && down.x >= last.x + last.w); /* right of the rows */
    CHECK(NEAR_EQ(up.y, first.y) && NEAR_EQ(down.y + down.h, last.y + last.h));
    CHECK(label.y >= up.y + up.h && label.y + label.h <= down.y);
    CHECK(!overlap(up, down) && !overlap(up, bottom_layout_button(BOTTOM_BUTTON_OPTIONS)));
    CHECK(first.w < bottom_layout_option_row(0, false).w);
    CHECK(NEAR_EQ(first.x, bottom_layout_option_row(0, false).x) && NEAR_EQ(first.y, bottom_layout_option_row(0, false).y));
    for (int row = 0; row < BOTTOM_LAYOUT_OPTION_ROWS; row++) {
        PxRect r = bottom_layout_option_row(row, true);
        CHECK(!overlap(r, up) && !overlap(r, down) && !overlap(r, label));
        CHECK(bottom_layout_option_at((int)(r.x + r.w / 2), (int)(r.y + r.h / 2), BOTTOM_LAYOUT_OPTION_ROWS, true) == row);
    }
    CHECK(bottom_layout_pager_at((int)(up.x + up.w / 2), (int)(up.y + up.h / 2)) == BOTTOM_PAGER_UP);
    CHECK(bottom_layout_pager_at((int)(down.x + down.w / 2), (int)(down.y + down.h / 2)) == BOTTOM_PAGER_DOWN);
    CHECK(bottom_layout_pager_at((int)(label.x + label.w / 2), (int)(label.y + label.h / 2)) == BOTTOM_PAGER_NONE);
    CHECK(bottom_layout_pager_at((int)(first.x + 4), (int)(first.y + 4)) == BOTTOM_PAGER_NONE);
    CHECK(bottom_layout_option_at((int)(up.x + 2), (int)(up.y + 2), BOTTOM_LAYOUT_OPTION_ROWS, true) == -1);
}

static void test_sheets(void)
{
    CHECK(bottom_layout_option_sheets(0) == 1);
    CHECK(bottom_layout_option_sheets(1) == 1);
    CHECK(bottom_layout_option_sheets(BOTTOM_LAYOUT_OPTION_ROWS) == 1);
    CHECK(bottom_layout_option_sheets(BOTTOM_LAYOUT_OPTION_ROWS + 1) == 2);
    CHECK(bottom_layout_option_sheets(2 * BOTTOM_LAYOUT_OPTION_ROWS) == 2);
    CHECK(bottom_layout_option_sheets(2 * BOTTOM_LAYOUT_OPTION_ROWS + 1) == 3);
    CHECK(bottom_layout_option_sheets(-3) == 1);
}

static void test_fit_whole_screen_is_the_menus_view(void)
{
    UnitBox menus = { -160, 160, -120, 120 };
    UnitBox v = bottom_layout_fit(menus, screen(), BOTTOM_SCREEN_W, BOTTOM_SCREEN_H);
    CHECK(NEAR_EQ(v.left, -160) && NEAR_EQ(v.right, 160) && NEAR_EQ(v.bottom, -120) && NEAR_EQ(v.top, 120));
    UnitBox window = { -136, 136, -102, 102 }; /* 4:3: fills the screen exactly */
    v = bottom_layout_fit(window, screen(), BOTTOM_SCREEN_W, BOTTOM_SCREEN_H);
    CHECK(NEAR_EQ(v.left, -136) && NEAR_EQ(v.right, 136) && NEAR_EQ(v.bottom, -102) && NEAR_EQ(v.top, 102));
}

static void test_fit_into_a_rectangle(void)
{
    /* 10 x 10 units into a 50 x 50 square at (20, 40): 5 px per unit */
    UnitBox src = { 0, 10, 0, 10 };
    PxRect dst = { 20, 40, 50, 50 };
    UnitBox v = bottom_layout_fit(src, dst, 320, 240);
    CHECK(NEAR_EQ(v.left, -4) && NEAR_EQ(v.right, 60));
    CHECK(NEAR_EQ(v.top, 18) && NEAR_EQ(v.bottom, -30));
}

static void test_fit_centres_at_one_scale(void)
{
    /* 10 x 10 units into 100 x 50 px: 5 px per unit, 25 px each side */
    UnitBox src = { 0, 10, 0, 10 };
    PxRect dst = { 0, 0, 100, 50 };
    UnitBox v = bottom_layout_fit(src, dst, 320, 240);
    CHECK(NEAR_EQ(v.left, -5) && NEAR_EQ(v.right, 59));
    CHECK(NEAR_EQ(v.top, 10) && NEAR_EQ(v.bottom, -38));
    float per_unit_x = 320 / (v.right - v.left);
    float per_unit_y = 240 / (v.top - v.bottom);
    CHECK(NEAR_EQ(per_unit_x, per_unit_y));
}

static void test_fit_without_area_gives_the_menus_view(void)
{
    UnitBox flat = { 0, 0, 0, 10 };
    PxRect dst = { 0, 0, 100, 50 };
    UnitBox v = bottom_layout_fit(flat, dst, 320, 240);
    CHECK(NEAR_EQ(v.left, -160) && NEAR_EQ(v.right, 160) && NEAR_EQ(v.bottom, -120) && NEAR_EQ(v.top, 120));
    UnitBox src = { 0, 10, 0, 10 };
    PxRect empty = { 0, 0, 0, 50 };
    v = bottom_layout_fit(src, empty, 320, 240);
    CHECK(NEAR_EQ(v.left, -160) && NEAR_EQ(v.top, 120));
}

/* The map menu's tiles (m_map_ovl.c, mMP_set_map_dl): 22 units apart, A-1
 * centred at (11.7, 45.7), rows going down. In the menus' own 1:1 view a
 * pixel is a unit, from the screen's centre. */
static void test_the_acre_under_a_touch(void)
{
    UnitBox menus = { -160, 160, -120, 120 };
    int col = -1;
    int row = -1;
    CHECK(bottom_layout_acre_at(menus, 172, 74, 5, 6, &col, &row) && col == 0 && row == 0);
    CHECK(bottom_layout_acre_at(menus, 160 + 12 + 22 * 4, 120 - 46 + 22 * 5, 5, 6, &col, &row) && col == 4 &&
          row == 5);
    CHECK(bottom_layout_acre_at(menus, 161, 74, 5, 6, &col, &row) && col == 0); /* A-1's left edge: unit 0.7 */
    CHECK(!bottom_layout_acre_at(menus, 160, 74, 5, 6, &col, &row));
    CHECK(bottom_layout_acre_at(menus, 270, 74, 5, 6, &col, &row) && col == 4); /* the right edge: 110.7 */
    CHECK(!bottom_layout_acre_at(menus, 271, 74, 5, 6, &col, &row));
    CHECK(bottom_layout_acre_at(menus, 172, 195, 5, 6, &col, &row) && row == 5); /* the bottom: -75.3 */
    CHECK(!bottom_layout_acre_at(menus, 172, 196, 5, 6, &col, &row));
    CHECK(!bottom_layout_acre_at(menus, 172, 63, 5, 6, &col, &row)); /* above A: 56.7 */
    CHECK(!bottom_layout_acre_at(menus, 172, 74, 5, 6, NULL, &row));
}

/* Each tile's centre, drawn where the grid goes, touches back to its tile. */
static void test_acres_where_the_grid_is_drawn(void)
{
    UnitBox grid_box = { -9.0f, 121.0f, -86.0f, 66.0f }; /* town_map_3ds.c */
    UnitBox v = bottom_layout_fit(grid_box, bottom_layout_grid(), BOTTOM_SCREEN_W, BOTTOM_SCREEN_H);
    for (int row = 0; row < 6; row++) {
        for (int col = 0; col < 5; col++) {
            float ux = 11.7f + 22.0f * (float)col;
            float uy = 45.7f - 22.0f * (float)row;
            int x = (int)((ux - v.left) / (v.right - v.left) * BOTTOM_SCREEN_W);
            int y = (int)((v.top - uy) / (v.top - v.bottom) * BOTTOM_SCREEN_H);
            int c = -1;
            int r = -1;
            CHECK(bottom_layout_acre_at(v, x, y, 5, 6, &c, &r) && c == col && r == row);
            CHECK(bottom_layout_button_at(x, y) == BOTTOM_BUTTON_NONE); /* the corner buttons are elsewhere */
        }
    }
}

int main(void)
{
    test_the_corner_buttons();
    test_touching_the_corner_buttons();
    test_the_map_fills_the_screen();
    test_option_rows();
    test_paged_rows_and_the_pager();
    test_sheets();
    test_fit_whole_screen_is_the_menus_view();
    test_fit_into_a_rectangle();
    test_fit_centres_at_one_scale();
    test_fit_without_area_gives_the_menus_view();
    test_the_acre_under_a_touch();
    test_acres_where_the_grid_is_drawn();
    if (failures != 0) {
        fprintf(stderr, "bottom_layout: %d check(s) failed\n", failures);
        return 1;
    }
    printf("bottom_layout: all tests passed\n");
    return 0;
}
