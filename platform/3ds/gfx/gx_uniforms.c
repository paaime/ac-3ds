#include "gx_uniforms.h"

#include <citro3d.h>
#include <string.h>

#define FV_UNIFORMS   96 /* the PICA's float uniform registers */
#define GX_TG_NRM     1
#define COLOR_SCALE   (1.0f / 255.0f) /* at most 1 float24 ulp from x / 255 */

static struct {
    int projection, model_view, normal, texmtx[GXU_TEXGENS], tex_src_nrm;
    int mat_color, amb_color, mat_select, lit_select;
    int light_pos, light_color;
} L;

static struct {
    float unif[FV_UNIFORMS][4];
    bool unif_valid[FV_UNIFORMS];
    uint64_t projection_stamp, matrices_stamp, texgen_stamp, lighting_stamp;
    GxViewportFold fold;
    GxUvAffine uv[GXU_TEXGENS];
} U;

bool gxu_locate(shaderInstance_s* vs)
{
    L.projection = shaderInstanceGetUniformLocation(vs, "projection");
    L.model_view = shaderInstanceGetUniformLocation(vs, "modelView");
    L.normal = shaderInstanceGetUniformLocation(vs, "normalMtx");
    L.texmtx[0] = shaderInstanceGetUniformLocation(vs, "texMtx0");
    L.texmtx[1] = shaderInstanceGetUniformLocation(vs, "texMtx1");
    L.tex_src_nrm = shaderInstanceGetUniformLocation(vs, "texSrcNrm");
    L.mat_color = shaderInstanceGetUniformLocation(vs, "matColor");
    L.amb_color = shaderInstanceGetUniformLocation(vs, "ambColor");
    L.mat_select = shaderInstanceGetUniformLocation(vs, "matSelect");
    L.lit_select = shaderInstanceGetUniformLocation(vs, "litSelect");
    L.light_pos = shaderInstanceGetUniformLocation(vs, "lightPos");
    L.light_color = shaderInstanceGetUniformLocation(vs, "lightColor");
    gxu_invalidate();
    return L.projection >= 0 && L.model_view >= 0 && L.normal >= 0 && L.texmtx[0] >= 0 && L.texmtx[1] >= 0 &&
           L.tex_src_nrm >= 0 && L.mat_color >= 0 && L.amb_color >= 0 && L.mat_select >= 0 &&
           L.lit_select >= 0 && L.light_pos >= 0 && L.light_color >= 0;
}

static void forget_groups(void)
{
    U.projection_stamp = 0;
    U.matrices_stamp = 0;
    U.texgen_stamp = 0;
    U.lighting_stamp = 0;
}

void gxu_invalidate(void)
{
    memset(U.unif_valid, 0, sizeof(U.unif_valid));
    forget_groups();
}

static void set_vec(int loc, float x, float y, float z, float w)
{
    if (loc < 0 || loc >= FV_UNIFORMS) {
        return;
    }
    float* c = U.unif[loc];
    if (U.unif_valid[loc] && c[0] == x && c[1] == y && c[2] == z && c[3] == w) {
        return;
    }
    c[0] = x;
    c[1] = y;
    c[2] = z;
    c[3] = w;
    U.unif_valid[loc] = true;
    C3D_FVUnifSet(GPU_VERTEX_SHADER, loc, x, y, z, w);
}

static void set_row(int loc, const float row[4])
{
    set_vec(loc, row[0], row[1], row[2], row[3]);
}

static void set_color(int loc, uint32_t rgba)
{
    set_vec(loc, (float)(rgba >> 24) * COLOR_SCALE, (float)((rgba >> 16) & 0xFF) * COLOR_SCALE,
            (float)((rgba >> 8) & 0xFF) * COLOR_SCALE, (float)(rgba & 0xFF) * COLOR_SCALE);
}

static bool same_stamp(uint64_t* last, uint64_t stamp)
{
    if (stamp != 0 && stamp == *last) {
        return true;
    }
    *last = stamp;
    return false;
}

static bool same_fold(GxViewportFold a, GxViewportFold b)
{
    return a.scale_x == b.scale_x && a.offset_x == b.offset_x && a.scale_y == b.scale_y && a.offset_y == b.offset_y &&
           a.widen_x == b.widen_x;
}

static bool same_uv(const GxUvAffine* a, const GxUvAffine* b)
{
    for (int r = 0; r < 2; r++) {
        for (int c = 0; c < 3; c++) {
            if (a->m[r][c] != b->m[r][c]) {
                return false;
            }
        }
    }
    return true;
}

/* Quarter-turn for the portrait screens, as Mtx_PerspTilt (x' = y, y' = -x); GX and PICA share depth [-1, 0]. */
static void upload_projection(const GxbState* st, GxViewportFold fold)
{
    bool fold_kept = same_fold(U.fold, fold);
    if (same_stamp(&U.projection_stamp, st->stamps.projection) && fold_kept) {
        return;
    }
    U.fold = fold;
    float r0[4], r1[4];
    for (int i = 0; i < 4; i++) {
        float widen = i == 0 ? fold.widen_x : 1.0f;
        float x = fold.scale_x * st->projection[0][i] * widen + fold.offset_x * st->projection[3][i];
        float y = fold.scale_y * st->projection[1][i] + fold.offset_y * st->projection[3][i];
        r0[i] = y;
        r1[i] = -x;
    }
    set_row(L.projection + 0, r0);
    set_row(L.projection + 1, r1);
    set_row(L.projection + 2, st->projection[2]);
    set_row(L.projection + 3, st->projection[3]);
}

static void upload_matrices(const GxbState* st)
{
    static const float LAST_ROW[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
    u32 slot = st->current_mtx / 3;
    if (slot >= GXB_POS_MATRICES) {
        slot = 0;
    }
    if (same_stamp(&U.matrices_stamp, st->stamps.matrices)) {
        return;
    }
    for (int i = 0; i < 3; i++) {
        set_row(L.model_view + i, st->pos_mtx[slot].m[i]);
        set_row(L.normal + i, st->nrm_mtx[slot].m[i]);
    }
    set_row(L.model_view + 3, LAST_ROW);
}

/* Two rows suffice: emu64's GX_TG_NRM matrix has last row (0, 0, 0, 1). */
static void upload_texgen(const GxbState* st, const GxUvAffine uv[GXU_TEXGENS])
{
    bool uv_kept = true;
    for (int n = 0; n < GXU_TEXGENS; n++) {
        uv_kept = uv_kept && same_uv(&U.uv[n], &uv[n]);
    }
    if (same_stamp(&U.texgen_stamp, st->stamps.texgen) && uv_kept) {
        return;
    }
    float from_normal[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    for (int n = 0; n < GXU_TEXGENS; n++) {
        float r0[4] = { 1.0f, 0.0f, 0.0f, 0.0f };
        float r1[4] = { 0.0f, 1.0f, 0.0f, 0.0f };
        u32 id = st->texgen[n].mtx;
        if (id >= GXB_TEXMTX_BASE && id < GXB_TEXMTX_IDENTITY) {
            const GxbMtx34* tm = &st->tex_mtx[(id - GXB_TEXMTX_BASE) / 3];
            memcpy(r0, tm->m[0], sizeof(r0));
            memcpy(r1, tm->m[1], sizeof(r1));
        }
        float o0[4], o1[4];
        gx_uv_apply(uv[n], r0, r1, o0, o1);
        set_row(L.texmtx[n] + 0, o0);
        set_row(L.texmtx[n] + 1, o1);
        from_normal[n] = st->texgen[n].src == GX_TG_NRM ? 1.0f : 0.0f;
        U.uv[n] = uv[n];
    }
    set_row(L.tex_src_nrm, from_normal);
}

static void upload_lighting(const GxbState* st)
{
    if (same_stamp(&U.lighting_stamp, st->stamps.lighting)) {
        return;
    }
    const GxbChannel* c = &st->chan[GXB_CHAN_COLOR0];
    const GxbChannel* a = &st->chan[GXB_CHAN_ALPHA0];
    float color_from_vtx = c->mat_src == GXB_SRC_VTX ? 1.0f : 0.0f;
    float alpha_from_vtx = a->mat_src == GXB_SRC_VTX ? 1.0f : 0.0f;
    float color_lit = c->enable ? 1.0f : 0.0f;
    float alpha_lit = a->enable ? 1.0f : 0.0f;
    set_vec(L.mat_select, color_from_vtx, color_from_vtx, color_from_vtx, alpha_from_vtx);
    set_vec(L.lit_select, color_lit, color_lit, color_lit, alpha_lit);
    set_color(L.mat_color, st->mat_color);
    set_color(L.amb_color, st->amb_color);
    for (int i = 0; i < GXB_MAX_LIGHTS; i++) {
        const GxbLight* l = &st->lights[i];
        bool on = c->enable && (c->light_mask & (1u << i));
        set_vec(L.light_pos + i, l->pos[0], l->pos[1], l->pos[2], 1.0f);
        set_color(L.light_color + i, on ? l->color : 0);
    }
}

void gxu_upload_draw(const GxbState* st, GxViewportFold fold, const GxUvAffine uv[GXU_TEXGENS])
{
    upload_projection(st, fold);
    upload_matrices(st);
    upload_texgen(st, uv);
    upload_lighting(st);
}

void gxu_upload_blit(void)
{
    static const float IDENTITY[4][4] = { { 1, 0, 0, 0 }, { 0, 1, 0, 0 }, { 0, 0, 1, 0 }, { 0, 0, 0, 1 } };
    static const float ROTATED0[4] = { 0, 1, 0, 0 }; /* the screen's quarter-turn, as upload_projection */
    static const float ROTATED1[4] = { -1, 0, 0, 0 };
    forget_groups();
    set_row(L.projection + 0, ROTATED0);
    set_row(L.projection + 1, ROTATED1);
    set_row(L.projection + 2, IDENTITY[2]);
    set_row(L.projection + 3, IDENTITY[3]);
    for (int i = 0; i < 4; i++) {
        set_row(L.model_view + i, IDENTITY[i]);
    }
    for (int i = 0; i < 3; i++) {
        set_row(L.normal + i, IDENTITY[i]);
    }
    set_row(L.texmtx[0] + 0, IDENTITY[0]);
    set_row(L.texmtx[0] + 1, IDENTITY[1]);
    set_vec(L.tex_src_nrm, 0, 0, 0, 0);
    set_vec(L.mat_select, 1, 1, 1, 1);
    set_vec(L.lit_select, 0, 0, 0, 0);
}
