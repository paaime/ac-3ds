/* Host tests for gx_screen.c. */
#include "gx_screen.h"

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

/* The top screen, 400 x 240 landscape; whole, or with the GameCube's 4:3
 * picture in its middle (320 x 240, 40 px bars). */
#define TOP    gx_screen_target(640.0f, 480.0f, 400, 240, false)
#define PILLAR gx_screen_target(640.0f, 480.0f, 400, 240, true)

static void test_full_screen_is_the_whole_target(void)
{
    GxScreenRect r = { 0, 0, 640, 480 };
    PicaRect p = gx_screen_to_pica(r, TOP);
    CHECK(p.x == 0 && p.y == 0 && p.w == 240 && p.h == 400);
    CHECK(gx_screen_is_full(r, TOP));
}

static void test_left_half_of_the_screen(void)
{
    /* The screen's left edge is the framebuffer's last row in viewport terms
     * (y = 400), its top edge the last column (x = 240). */
    GxScreenRect r = { 0, 0, 320, 480 };
    PicaRect p = gx_screen_to_pica(r, TOP);
    CHECK(p.x == 0 && p.w == 240);
    CHECK(p.y == 200 && p.h == 200);
    CHECK(!gx_screen_is_full(r, TOP));
}

static void test_top_half_of_the_screen(void)
{
    GxScreenRect r = { 0, 0, 640, 240 };
    PicaRect p = gx_screen_to_pica(r, TOP);
    CHECK(p.x == 120 && p.w == 120);
    CHECK(p.y == 0 && p.h == 400);
}

static void test_clamps_to_the_target(void)
{
    GxScreenRect r = { -100, -50, 1000, 1000 };
    PicaRect p = gx_screen_to_pica(r, TOP);
    CHECK(p.x == 0 && p.y == 0 && p.w == 240 && p.h == 400);
    GxScreenRect off = { 700, 0, 50, 50 }; /* entirely off screen */
    PicaRect q = gx_screen_to_pica(off, TOP);
    CHECK(q.w == 0 || q.h == 0);
}

static void test_inner_rectangle(void)
{
    /* x 160..480 of 640 -> viewport y 100..300; y 120..360 of 480 -> viewport x 60..180 */
    GxScreenRect r = { 160, 120, 320, 240 };
    PicaRect p = gx_screen_to_pica(r, TOP);
    CHECK(p.x == 60 && p.w == 120);
    CHECK(p.y == 100 && p.h == 200);
}

#define NEAR_EQ(a, b) ((a) - (b) < 1e-5f && (b) - (a) < 1e-5f)

static void test_full_viewport_folds_to_identity(void)
{
    GxViewportFold f = gx_screen_viewport_fold((GxScreenRect){ 0, 0, 640, 480 }, TOP);
    CHECK(NEAR_EQ(f.scale_x, 1.0f) && NEAR_EQ(f.scale_y, 1.0f));
    CHECK(NEAR_EQ(f.offset_x, 0.0f) && NEAR_EQ(f.offset_y, 0.0f));
}

static void test_left_half_viewport_folds_into_the_left_half(void)
{
    /* NDC x = 1 (the viewport's right edge) lands at the screen's centre. */
    GxViewportFold f = gx_screen_viewport_fold((GxScreenRect){ 0, 0, 320, 480 }, TOP);
    CHECK(NEAR_EQ(f.scale_x * 1.0f + f.offset_x, 0.0f));
    CHECK(NEAR_EQ(f.scale_x * -1.0f + f.offset_x, -1.0f));
    CHECK(NEAR_EQ(f.scale_y, 1.0f) && NEAR_EQ(f.offset_y, 0.0f));
}

static void test_partly_off_screen_viewport_keeps_its_scale(void)
{
    /* A 640-wide viewport starting at x = -320 shows its right half at full
     * scale (cropped, not squashed into the visible part). */
    GxViewportFold f = gx_screen_viewport_fold((GxScreenRect){ -320, 0, 640, 480 }, TOP);
    CHECK(NEAR_EQ(f.scale_x, 1.0f));
    CHECK(NEAR_EQ(f.scale_x * 0.0f + f.offset_x, -1.0f)); /* its centre is the screen's left edge */
}

static void test_top_half_viewport_moves_up(void)
{
    GxViewportFold f = gx_screen_viewport_fold((GxScreenRect){ 0, 0, 640, 240 }, TOP);
    CHECK(NEAR_EQ(f.scale_y, 0.5f));
    CHECK(NEAR_EQ(f.scale_y * -1.0f + f.offset_y, 0.0f)); /* its bottom edge is mid-screen (NDC y up) */
}

static void test_target_shapes(void)
{
    GxScreenTarget t = TOP;
    CHECK(t.fb_width == 240 && t.fb_height == 400); /* portrait */
    CHECK(t.image_x == 0 && t.image_y == 0 && t.image_w == 400 && t.image_h == 240);
    GxScreenTarget p = PILLAR;
    CHECK(p.image_x == 40 && p.image_y == 0 && p.image_w == 320 && p.image_h == 240);
    GxScreenTarget b = gx_screen_target(640.0f, 480.0f, 320, 240, true); /* already 4:3: no bars */
    CHECK(b.image_x == 0 && b.image_w == 320 && b.image_h == 240);
}

static void test_pillarbox_maps_the_frame_to_the_middle(void)
{
    GxScreenRect r = { 0, 0, 640, 480 };
    PicaRect p = gx_screen_to_pica(r, PILLAR);
    CHECK(p.x == 0 && p.w == 240 && p.y == 40 && p.h == 320);
    CHECK(!gx_screen_is_full(r, PILLAR)); /* the bars stay out: the scissor stays on */
    GxScreenRect left = { 0, 0, 320, 480 }; /* landscape x 40..200 */
    PicaRect q = gx_screen_to_pica(left, PILLAR);
    CHECK(q.y == 200 && q.h == 160);
    GxScreenRect big = { -100, -50, 1000, 1000 };
    PicaRect c = gx_screen_to_pica(big, PILLAR);
    CHECK(c.x == 0 && c.y == 40 && c.w == 240 && c.h == 320);
}

static void test_pillarbox_folds_into_the_middle(void)
{
    GxViewportFold f = gx_screen_viewport_fold((GxScreenRect){ 0, 0, 640, 480 }, PILLAR);
    CHECK(NEAR_EQ(f.scale_x, 0.8f) && NEAR_EQ(f.offset_x, 0.0f));
    CHECK(NEAR_EQ(f.scale_y, 1.0f) && NEAR_EQ(f.offset_y, 0.0f));
    GxViewportFold h = gx_screen_viewport_fold((GxScreenRect){ 0, 0, 320, 480 }, PILLAR);
    CHECK(NEAR_EQ(h.scale_x * 1.0f + h.offset_x, 0.0f));   /* the viewport's right edge: mid-screen */
    CHECK(NEAR_EQ(h.scale_x * -1.0f + h.offset_x, -0.8f)); /* its left edge: the bar's edge */
}

static void test_intersection(void)
{
    GxScreenRect a = { 0, 0, 320, 480 };
    GxScreenRect b = { 100, 100, 640, 100 };
    GxScreenRect c = gx_screen_intersect(a, b);
    CHECK(c.left == 100 && c.top == 100 && c.width == 220 && c.height == 100);
    GxScreenRect d = gx_screen_intersect(a, (GxScreenRect){ 400, 0, 100, 100 });
    CHECK(d.width == 0 || d.height == 0);
}

#define NEAR(a, b) ((a) - (b) < 0.001f && (b) - (a) < 0.001f)

/* The top screen is 5:3 and the GameCube's picture 4:3: drawn at its own
 * proportions, a 4:3 picture takes 0.8 of the width. A target no wider than
 * the picture needs no correction. */
static void test_wide_factor(void)
{
    CHECK(NEAR(gx_screen_wide_factor(TOP), 0.8f));
    CHECK(NEAR(gx_screen_wide_factor(PILLAR), 1.0f));
    CHECK(NEAR(gx_screen_wide_factor(gx_screen_target(640.0f, 480.0f, 240, 240, false)), 1.0f));
    CHECK(NEAR(gx_screen_wide_factor(gx_screen_target(640.0f, 480.0f, 320, 240, false)), 1.0f));
}

/* As the PC port: the world (untagged draws) is seen wider, perspective or
 * not; menus keep their proportions, so their 2D is narrowed too; full-screen
 * transitions and pictures stay stretched over the whole screen. */
static void test_projection_widen(void)
{
    CHECK(NEAR(gx_screen_projection_widen(GX_WIDE_MORE_VIEW, false, 0.8f), 0.8f));
    CHECK(NEAR(gx_screen_projection_widen(GX_WIDE_MORE_VIEW, true, 0.8f), 0.8f));
    CHECK(NEAR(gx_screen_projection_widen(GX_WIDE_STRETCHED, false, 0.8f), 1.0f));
    CHECK(NEAR(gx_screen_projection_widen(GX_WIDE_STRETCHED, true, 0.8f), 1.0f));
    CHECK(NEAR(gx_screen_projection_widen(GX_WIDE_UI, true, 0.8f), 0.8f));
    CHECK(NEAR(gx_screen_projection_widen(GX_WIDE_UI, false, 0.8f), 1.0f));
    CHECK(NEAR(gx_screen_projection_widen(GX_WIDE_MORE_VIEW, false, 1.0f), 1.0f));
}

/* A menu's viewport other than the whole screen goes where the narrowed menu
 * puts it: into the 4:3 middle (64 EFB pixels in, at 0.8), as wide as before
 * times 0.8. The whole screen, and an unwidened target, stay as they are. */
static void test_ui_viewport(void)
{
    GxScreenRect full = { 0, 0, 640, 480 };
    GxScreenRect f = gx_screen_ui_viewport(full, 0.8f, 640.0f, 480.0f);
    CHECK(f.left == 0 && f.top == 0 && f.width == 640 && f.height == 480);
    GxScreenRect icon = { 100, 50, 64, 32 };
    GxScreenRect r = gx_screen_ui_viewport(icon, 0.8f, 640.0f, 480.0f);
    CHECK(NEAR(r.left, 144.0f) && NEAR(r.width, 51.2f));
    CHECK(r.top == 50 && r.height == 32);
    GxScreenRect same = gx_screen_ui_viewport(icon, 1.0f, 640.0f, 480.0f);
    CHECK(same.left == 100 && same.width == 64);
    GxScreenRect almost = { 0.5f, 0.5f, 639.6f, 479.6f }; /* the whole screen, give or take rounding */
    CHECK(gx_screen_ui_viewport(almost, 0.8f, 640.0f, 480.0f).width == almost.width);
}

/* A fold leaves the projection's own x scale as it is; the renderer widens it. */
static void test_fold_starts_unwidened(void)
{
    GxScreenRect v = { 0, 0, 640, 480 };
    CHECK(gx_screen_viewport_fold(v, TOP).widen_x == 1.0f);
    CHECK(gx_screen_viewport_fold(v, PILLAR).widen_x == 1.0f);
}

int main(void)
{
    test_full_screen_is_the_whole_target();
    test_left_half_of_the_screen();
    test_top_half_of_the_screen();
    test_clamps_to_the_target();
    test_inner_rectangle();
    test_full_viewport_folds_to_identity();
    test_left_half_viewport_folds_into_the_left_half();
    test_partly_off_screen_viewport_keeps_its_scale();
    test_top_half_viewport_moves_up();
    test_intersection();
    test_target_shapes();
    test_pillarbox_maps_the_frame_to_the_middle();
    test_pillarbox_folds_into_the_middle();
    test_wide_factor();
    test_projection_widen();
    test_ui_viewport();
    test_fold_starts_unwidened();
    if (failures == 0) {
        printf("gx_screen: all tests passed\n");
    }
    return failures == 0 ? 0 : 1;
}
