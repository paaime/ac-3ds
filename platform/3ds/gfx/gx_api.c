#include <dolphin/gx.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "gx_render.h"
#include "gx_screen.h"
#include "gx_state.h"

_Static_assert(sizeof(GxbTexObj) <= sizeof(GXTexObj), "GxbTexObj must fit in GXTexObj");
_Static_assert(sizeof(GxbTlut) <= sizeof(GXTlutObj), "GxbTlut must fit in GXTlutObj");
_Static_assert(sizeof(GxbLight) <= sizeof(GXLightObj), "GxbLight must fit in GXLightObj");

static GxbState s_state;
static float s_tex0_scale = 1.0f;

/* emu64's TARGET_PC paths read and write these to detect palette changes. */
u16 s_tlut_first_word[16];

enum {
    WARN_SWAP_TABLE,
    WARN_INDIRECT,
    WARN_UNFINISHED_BATCH,
    WARN_COUNT,
};

static bool s_warned[WARN_COUNT];

static void warn_once(int id, const char* format, ...)
{
    if (s_warned[id]) {
        return;
    }
    s_warned[id] = true;
    va_list args;
    va_start(args, format);
    fputs("[gx] ", stderr);
    vfprintf(stderr, format, args);
    fputc('\n', stderr);
    va_end(args);
}

static u32 pack_color(GXColor c)
{
    return gxb_rgba(c.r, c.g, c.b, c.a);
}

static void identity34(GxbMtx34* m)
{
    memset(m, 0, sizeof(*m));
    m->m[0][0] = m->m[1][1] = m->m[2][2] = 1.0f;
}

static uint64_t s_last_stamp;

static void touch(uint64_t* stamp)
{
    *stamp = ++s_last_stamp;
}

static void update(void* dst, const void* src, size_t size, uint64_t* stamp)
{
    if (memcmp(dst, src, size) != 0) {
        memcpy(dst, src, size);
        touch(stamp);
    }
}

static void touch_all(GxbState* st)
{
    touch(&st->stamps.projection);
    touch(&st->stamps.matrices);
    touch(&st->stamps.texgen);
    touch(&st->stamps.lighting);
    touch(&st->stamps.tev);
}

static void reset_state(void)
{
    memset(&s_state, 0, sizeof(s_state));
    for (int i = 0; i < GXB_POS_MATRICES; i++) {
        identity34(&s_state.pos_mtx[i]);
        identity34(&s_state.nrm_mtx[i]);
    }
    for (int i = 0; i < GXB_TEX_MATRICES; i++) {
        identity34(&s_state.tex_mtx[i]);
    }
    for (int i = 0; i < 4; i++) {
        s_state.projection[i][i] = 1.0f;
    }
    s_state.projection_type = GXB_PROJ_ORTHOGRAPHIC;
    s_state.color_update = true;
    s_state.alpha_update = true;
    s_state.z_enable = true;
    s_state.z_func = GX_LEQUAL;
    s_state.z_update = true;
    s_state.alpha_comp0 = GX_ALWAYS;
    s_state.alpha_comp1 = GX_ALWAYS;
    s_state.cull_mode = GXB_CULL_BACK;
    s_state.tev.stage_count = 1;
    for (int i = 0; i < GX_TEV_MAX_STAGES; i++) {
        GxTevStage* st = &s_state.tev.stages[i];
        st->tex_coord = GX_TEV_NONE;
        st->tex_map = GX_TEV_NONE;
    }
    s_state.has_color = true;
    /* the whole EFB: a zero-size scissor draws nothing */
    const float full_viewport[4] = { 0.0f, 0.0f, GXB_EFB_WIDTH, GXB_EFB_HEIGHT };
    const uint32_t full_scissor[4] = { 0, 0, 640, 480 };
    memcpy(s_state.viewport, full_viewport, sizeof(full_viewport));
    memcpy(s_state.scissor, full_scissor, sizeof(full_scissor));
    touch_all(&s_state);
}

static bool s_initialised;

static GxbState* state(void)
{
    if (!s_initialised) {
        reset_state();
        s_initialised = true;
    }
    return &s_state;
}

void GXBegin(GXPrimitive type, GXVtxFmt vtxfmt, u16 nverts)
{
    (void)vtxfmt;
    GxbState* st = state();
    if (gxr_batch_open()) {
        warn_once(WARN_UNFINISHED_BATCH, "GXBegin while a batch is open; flushing it");
        gxr_end(st);
    }
    gxr_begin((uint8_t)type, nverts);
}

void GXEnd(void)
{
    gxr_end(state());
}

void GXPosition3f32(f32 x, f32 y, f32 z)
{
    gxr_position(x, y, z);
}

void GXPosition2f32(f32 x, f32 y)
{
    gxr_position(x, y, 0.0f);
}

/* No fractional scaling (as pc_gx.c): emu64 and J2D send plain coordinates. */
void GXPosition2s16(s16 x, s16 y)
{
    gxr_position((f32)x, (f32)y, 0.0f);
}

void GXPosition2u16(u16 x, u16 y)
{
    gxr_position((f32)x, (f32)y, 0.0f);
}

void GXNormal3f32(f32 x, f32 y, f32 z)
{
    gxr_normal(x, y, z);
}

void GXColor4u8(u8 r, u8 g, u8 b, u8 a)
{
    gxr_color(r, g, b, a);
}

void GXColor1u32(u32 clr)
{
    gxr_color((u8)(clr >> 24), (u8)(clr >> 16), (u8)(clr >> 8), (u8)clr);
}

void GXTexCoord2f32(f32 s, f32 t)
{
    gxr_texcoord(s, t);
}

void GXTexCoord2u8(u8 s, u8 t)
{
    gxr_texcoord((float)s, (float)t);
}

void GXTexCoord2s16(s16 s, s16 t)
{
    gxr_texcoord((float)s * s_tex0_scale, (float)t * s_tex0_scale);
}

void GXClearVtxDesc(void)
{
    GxbState* st = state();
    st->has_normal = false;
    st->has_color = false;
    st->has_texcoord = false;
}

void GXSetVtxDesc(GXAttr attr, GXAttrType type)
{
    GxbState* st = state();
    bool present = type != GX_NONE;
    switch (attr) {
    case GX_VA_NRM: st->has_normal = present; break;
    case GX_VA_CLR0: st->has_color = present; break;
    case GX_VA_TEX0: st->has_texcoord = present; break;
    default: break;
    }
}

void GXSetVtxAttrFmt(GXVtxFmt vtxfmt, GXAttr attr, GXCompCnt cnt, GXCompType type, u8 frac)
{
    (void)vtxfmt;
    (void)cnt;
    (void)type;
    if (attr == GX_VA_TEX0) {
        s_tex0_scale = 1.0f / (float)(1u << (frac < 16 ? frac : 0));
    }
}

void GXInvalidateVtxCache(void) {}

static uint8_t wide_mark(void)
{
    int mark = g_pc_widescreen_stretch;
    return (uint8_t)(mark == GX_WIDE_STRETCHED || mark == GX_WIDE_UI ? mark : GX_WIDE_MORE_VIEW);
}

void GXSetProjection(const void* mtx, GXProjectionType type)
{
    GxbState* st = state();
    float projection[4][4];
    memcpy(projection, mtx, sizeof(float) * 12);
    float row3_persp[4] = { 0.0f, 0.0f, -1.0f, 0.0f };
    float row3_ortho[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
    memcpy(projection[3], type == GX_PERSPECTIVE ? row3_persp : row3_ortho, sizeof(row3_persp));
    update(st->projection, projection, sizeof(projection), &st->stamps.projection);
    st->projection_type = type == GX_PERSPECTIVE ? GXB_PROJ_PERSPECTIVE : GXB_PROJ_ORTHOGRAPHIC;
    uint8_t mark = wide_mark();
    if (st->projection_mark != mark) {
        st->projection_mark = mark;
        touch(&st->stamps.projection);
    }
}

void GXLoadPosMtxImm(const void* mtx, u32 id)
{
    u32 slot = id / 3;
    if (slot < GXB_POS_MATRICES) {
        GxbState* st = state();
        update(&st->pos_mtx[slot], mtx, sizeof(GxbMtx34), &st->stamps.matrices);
    }
}

void GXLoadNrmMtxImm(const void* mtx, u32 id)
{
    u32 slot = id / 3;
    if (slot < GXB_POS_MATRICES) {
        GxbState* st = state();
        update(&st->nrm_mtx[slot], mtx, sizeof(GxbMtx34), &st->stamps.matrices);
    }
}

void GXLoadTexMtxImm(const void* mtx, u32 id, GXTexMtxType type)
{
    if (id < GXB_TEXMTX_BASE || id >= GXB_TEXMTX_IDENTITY) {
        return; /* post-transform matrices aren't used by emu64 */
    }
    u32 slot = (id - GXB_TEXMTX_BASE) / 3;
    GxbMtx34 m;
    identity34(&m);
    memcpy(&m, mtx, sizeof(float) * 4 * (type == GX_MTX2x4 ? 2 : 3));
    GxbState* st = state();
    update(&st->tex_mtx[slot], &m, sizeof(m), &st->stamps.texgen);
}

void GXSetCurrentMtx(u32 id)
{
    GxbState* st = state();
    update(&st->current_mtx, &id, sizeof(id), &st->stamps.matrices);
}

void GXSetTexCoordGen2(GXTexCoordID dst, GXTexGenType func, GXTexGenSrc src, u32 mtx, GXBool normalize,
                       u32 postmtx)
{
    (void)func; /* emu64 only generates 2x4 matrix texcoords */
    (void)normalize;
    (void)postmtx;
    if (dst < GXB_MAX_TEXGENS) {
        GxbState* st = state();
        if (st->texgen[dst].src != (uint8_t)src || st->texgen[dst].mtx != mtx) {
            st->texgen[dst].src = (uint8_t)src;
            st->texgen[dst].mtx = mtx;
            touch(&st->stamps.texgen);
        }
    }
}

/* The shader always outputs both texcoords; the combiner samples what it uses. */
void GXSetNumTexGens(u8 nTexGens)
{
    (void)nTexGens;
}

/* The depth range stays [0, 1]: emu64 always passes it. */
void GXSetViewport(f32 left, f32 top, f32 wd, f32 ht, f32 nearz, f32 farz)
{
    (void)nearz;
    (void)farz;
    GxbState* st = state();
    st->viewport[0] = left;
    st->viewport[1] = top;
    st->viewport[2] = wd;
    st->viewport[3] = ht;
    st->viewport_mark = wide_mark();
}

void GXSetScissor(u32 left, u32 top, u32 wd, u32 ht)
{
    GxbState* st = state();
    st->scissor[0] = left;
    st->scissor[1] = top;
    st->scissor[2] = wd;
    st->scissor[3] = ht;
}

void GXSetScissorBoxOffset(s32 x_off, s32 y_off)
{
    (void)x_off;
    (void)y_off;
}

void GXSetClipMode(GXClipMode mode)
{
    (void)mode;
}

void GXInitLightPos(GXLightObj* lt_obj, f32 x, f32 y, f32 z)
{
    GxbLight* l = (GxbLight*)lt_obj;
    l->pos[0] = x;
    l->pos[1] = y;
    l->pos[2] = z;
}

/* The shader lights by position only (gx.v.pica). */
void GXInitLightDir(GXLightObj* lt_obj, f32 nx, f32 ny, f32 nz)
{
    (void)lt_obj;
    (void)nx;
    (void)ny;
    (void)nz;
}

void GXInitLightAttn(GXLightObj* lt_obj, f32 a0, f32 a1, f32 a2, f32 k0, f32 k1, f32 k2)
{
    /* emu64 only attenuates G_LIGHTING_POSITIONAL lights, which no static display list uses. */
    (void)lt_obj;
    (void)a0;
    (void)a1;
    (void)a2;
    (void)k0;
    (void)k1;
    (void)k2;
}

void GXInitLightColor(GXLightObj* lt_obj, GXColor color)
{
    ((GxbLight*)lt_obj)->color = pack_color(color);
}

void GXLoadLightObjImm(GXLightObj* lt_obj, GXLightID light)
{
    for (int i = 0; i < GXB_MAX_LIGHTS; i++) {
        if ((u32)light & (1u << i)) {
            GxbState* st = state();
            update(&st->lights[i], lt_obj, sizeof(GxbLight), &st->stamps.lighting);
        }
    }
}

void GXSetNumChans(u8 nChans)
{
    (void)nChans;
}

void GXSetChanCtrl(GXChannelID chan, GXBool enable, GXColorSrc amb_src, GXColorSrc mat_src, u32 light_mask,
                   GXDiffuseFn diff_fn, GXAttnFn attn_fn)
{
    (void)diff_fn; /* emu64 uses GX_DF_CLAMP or nothing; the shader clamps */
    (void)attn_fn;
    GxbChannel c;
    memset(&c, 0, sizeof(c)); /* padding too: update() compares bytes */
    c.enable = enable != 0;
    c.amb_src = (uint8_t)amb_src;
    c.mat_src = (uint8_t)mat_src;
    c.light_mask = light_mask;
    GxbState* st = state();
    if (chan == GX_COLOR0 || chan == GX_COLOR0A0) {
        update(&st->chan[GXB_CHAN_COLOR0], &c, sizeof(c), &st->stamps.lighting);
    }
    if (chan == GX_ALPHA0 || chan == GX_COLOR0A0) {
        update(&st->chan[GXB_CHAN_ALPHA0], &c, sizeof(c), &st->stamps.lighting);
    }
}

static u32 merge_channel_color(u32 current, GXChannelID chan, GXColor color)
{
    u32 value = pack_color(color);
    if (chan == GX_COLOR0) {
        return (value & 0xFFFFFF00u) | (current & 0xFFu);
    }
    if (chan == GX_ALPHA0) {
        return (current & 0xFFFFFF00u) | (value & 0xFFu);
    }
    return chan == GX_COLOR0A0 ? value : current;
}

void GXSetChanAmbColor(GXChannelID chan, GXColor amb_color)
{
    GxbState* st = state();
    u32 value = merge_channel_color(st->amb_color, chan, amb_color);
    update(&st->amb_color, &value, sizeof(value), &st->stamps.lighting);
}

void GXSetChanMatColor(GXChannelID chan, GXColor mat_color)
{
    GxbState* st = state();
    u32 value = merge_channel_color(st->mat_color, chan, mat_color);
    update(&st->mat_color, &value, sizeof(value), &st->stamps.lighting);
}

/* Marks the TEV changed: only for a setter about to write the stage. */
static GxTevStage* tev_stage(GXTevStageID stage)
{
    if (stage >= GX_TEV_MAX_STAGES) {
        return NULL;
    }
    GxbState* st = state();
    touch(&st->stamps.tev);
    return &st->tev.stages[stage];
}

void GXSetNumTevStages(u8 nStages)
{
    GxbState* st = state();
    int count = nStages;
    update(&st->tev.stage_count, &count, sizeof(count), &st->stamps.tev);
}

void GXSetTevColorIn(GXTevStageID stage, GXTevColorArg a, GXTevColorArg b, GXTevColorArg c, GXTevColorArg d)
{
    GxTevStage* s = tev_stage(stage);
    if (s != NULL) {
        uint8_t in[4] = { (uint8_t)a, (uint8_t)b, (uint8_t)c, (uint8_t)d };
        memcpy(s->color.in, in, sizeof(in));
    }
}

void GXSetTevAlphaIn(GXTevStageID stage, GXTevAlphaArg a, GXTevAlphaArg b, GXTevAlphaArg c, GXTevAlphaArg d)
{
    GxTevStage* s = tev_stage(stage);
    if (s != NULL) {
        uint8_t in[4] = { (uint8_t)a, (uint8_t)b, (uint8_t)c, (uint8_t)d };
        memcpy(s->alpha.in, in, sizeof(in));
    }
}

static void set_tev_op(GxTevChannel* ch, GXTevOp op, GXTevBias bias, GXTevScale scale, GXBool clamp,
                       GXTevRegID out_reg)
{
    ch->op = (uint8_t)op;
    ch->bias = (uint8_t)bias;
    ch->scale = (uint8_t)scale;
    ch->clamp = clamp != 0;
    ch->out_reg = (uint8_t)out_reg;
}

void GXSetTevColorOp(GXTevStageID stage, GXTevOp op, GXTevBias bias, GXTevScale scale, GXBool clamp,
                     GXTevRegID out_reg)
{
    GxTevStage* s = tev_stage(stage);
    if (s != NULL) {
        set_tev_op(&s->color, op, bias, scale, clamp, out_reg);
    }
}

void GXSetTevAlphaOp(GXTevStageID stage, GXTevOp op, GXTevBias bias, GXTevScale scale, GXBool clamp,
                     GXTevRegID out_reg)
{
    GxTevStage* s = tev_stage(stage);
    if (s != NULL) {
        set_tev_op(&s->alpha, op, bias, scale, clamp, out_reg);
    }
}

/* As in the SDK, stages after the first take the previous stage's output instead of the rasterised colour. */
void GXSetTevOp(GXTevStageID id, GXTevMode mode)
{
    GxTevStage* s = tev_stage(id);
    if (s == NULL) {
        return;
    }
    uint8_t carg = id == GX_TEVSTAGE0 ? GXT_CC_RASC : GXT_CC_CPREV;
    uint8_t aarg = id == GX_TEVSTAGE0 ? GXT_CA_RASA : GXT_CA_APREV;
    uint8_t c[4] = { GXT_CC_ZERO, GXT_CC_ZERO, GXT_CC_ZERO, carg };
    uint8_t a[4] = { GXT_CA_ZERO, GXT_CA_ZERO, GXT_CA_ZERO, aarg };
    switch (mode) {
    case GX_MODULATE:
        c[1] = GXT_CC_TEXC; c[2] = carg; c[3] = GXT_CC_ZERO;
        a[1] = GXT_CA_TEXA; a[2] = aarg; a[3] = GXT_CA_ZERO;
        break;
    case GX_DECAL:
        c[0] = carg; c[1] = GXT_CC_TEXC; c[2] = GXT_CC_TEXA; c[3] = GXT_CC_ZERO;
        break;
    case GX_BLEND:
        c[0] = carg; c[1] = GXT_CC_ONE; c[2] = GXT_CC_TEXC; c[3] = GXT_CC_ZERO;
        a[1] = GXT_CA_TEXA; a[2] = aarg; a[3] = GXT_CA_ZERO;
        break;
    case GX_REPLACE:
        c[3] = GXT_CC_TEXC;
        a[3] = GXT_CA_TEXA;
        break;
    default:
        break;
    }
    memcpy(s->color.in, c, sizeof(c));
    memcpy(s->alpha.in, a, sizeof(a));
    set_tev_op(&s->color, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
    set_tev_op(&s->alpha, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
}

void GXSetTevOrder(GXTevStageID stage, GXTexCoordID coord, GXTexMapID map, GXChannelID color)
{
    (void)color; /* emu64 always uses GX_COLOR0A0 (or NULL with no RAS inputs) */
    GxTevStage* s = tev_stage(stage);
    if (s != NULL) {
        s->tex_coord = coord == GX_TEXCOORD_NULL ? GX_TEV_NONE : (uint8_t)coord;
        bool no_map = map == GX_TEXMAP_NULL || ((u32)map & GX_TEX_DISABLE) != 0 || (u32)map >= GXB_MAX_TEXMAPS;
        s->tex_map = no_map ? GX_TEV_NONE : (uint8_t)map;
    }
}

/* As pc_gx.c: emu64 passes the colour registers as EmuColor.raw (0xRRGGBBAA), so bytes are reinterpreted. */
void GXSetTevColor(GXTevRegID id, GXColor color)
{
    if ((u32)id >= 4) {
        return;
    }
    u32 value = pack_color(color);
    if (id != GX_TEVREG0) {
        memcpy(&value, &color, sizeof(value));
    }
    GxbState* st = state();
    update(&st->tev.reg[id], &value, sizeof(value), &st->stamps.tev);
}

void GXSetTevKColorSel(GXTevStageID stage, GXTevKColorSel sel)
{
    GxTevStage* s = tev_stage(stage);
    if (s != NULL) {
        s->kcolor_sel = (uint8_t)sel;
    }
}

void GXSetTevKAlphaSel(GXTevStageID stage, GXTevKAlphaSel sel)
{
    GxTevStage* s = tev_stage(stage);
    if (s != NULL) {
        s->kalpha_sel = (uint8_t)sel;
    }
}

void GXSetTevSwapMode(GXTevStageID stage, GXTevSwapSel ras_sel, GXTevSwapSel tex_sel)
{
    (void)stage;
    if (ras_sel != GX_TEV_SWAP0 || tex_sel != GX_TEV_SWAP0) {
        warn_once(WARN_SWAP_TABLE, "TEV swap tables other than SWAP0 are ignored");
    }
}

void GXSetTevSwapModeTable(GXTevSwapSel table, GXTevColorChan red, GXTevColorChan green, GXTevColorChan blue,
                           GXTevColorChan alpha)
{
    (void)table;
    (void)red;
    (void)green;
    (void)blue;
    (void)alpha;
}

void GXSetTevDirect(GXTevStageID tev_stage_id)
{
    (void)tev_stage_id;
}

void GXSetNumIndStages(u8 nIndStages)
{
    if (nIndStages != 0) {
        warn_once(WARN_INDIRECT, "indirect texturing isn't supported on PICA200");
    }
}

void GXSetIndTexCoordScale(GXIndTexStageID ind_state, GXIndTexScale scale_s, GXIndTexScale scale_t)
{
    (void)ind_state;
    (void)scale_s;
    (void)scale_t;
}

static void init_tex_obj(GXTexObj* obj, void* image_ptr, u16 width, u16 height, u8 format, bool is_ci,
                         u32 tlut_name, GXTexWrapMode wrap_s, GXTexWrapMode wrap_t)
{
    GxbTexObj t = {
        .data = image_ptr,
        .width = width,
        .height = height,
        .format = format,
        .is_ci = is_ci,
        .tlut_name = (uint8_t)(tlut_name & 0xFF),
        .wrap_s = (uint8_t)wrap_s,
        .wrap_t = (uint8_t)wrap_t,
        .min_filter = GX_LINEAR,
        .mag_filter = GX_LINEAR,
        .valid = image_ptr != NULL,
    };
    memset(obj, 0, sizeof(*obj));
    memcpy(obj, &t, sizeof(t));
}

void GXInitTexObj(GXTexObj* obj, void* image_ptr, u16 width, u16 height, GXTexFmt format,
                  GXTexWrapMode wrap_s, GXTexWrapMode wrap_t, u8 mipmap)
{
    (void)mipmap; /* AC never mipmaps */
    init_tex_obj(obj, image_ptr, width, height, (u8)format, false, 0, wrap_s, wrap_t);
}

void GXInitTexObjCI(GXTexObj* obj, void* image_ptr, u16 width, u16 height, GXCITexFmt format,
                    GXTexWrapMode wrap_s, GXTexWrapMode wrap_t, u8 mipmap, u32 tlut_name)
{
    (void)mipmap;
    init_tex_obj(obj, image_ptr, width, height, (u8)format, true, tlut_name, wrap_s, wrap_t);
}

void GXInitTexObjLOD(GXTexObj* obj, GXTexFilter min_filt, GXTexFilter mag_filt, f32 min_lod, f32 max_lod,
                     f32 lod_bias, GXBool bias_clamp, GXBool do_edge_lod, GXAnisotropy max_aniso)
{
    (void)min_lod;
    (void)max_lod;
    (void)lod_bias;
    (void)bias_clamp;
    (void)do_edge_lod;
    (void)max_aniso;
    GxbTexObj* t = (GxbTexObj*)obj;
    t->min_filter = (uint8_t)min_filt;
    t->mag_filter = (uint8_t)mag_filt;
}

void GXLoadTexObj(GXTexObj* obj, GXTexMapID id)
{
    if ((u32)id < GXB_MAX_TEXMAPS) {
        state()->texmap[id] = *(const GxbTexObj*)obj;
    }
}

void GXInitTlutObj(GXTlutObj* tlut_obj, void* lut, GXTlutFmt fmt, u16 n_entries)
{
    GxbTlut t = { lut, (uint8_t)fmt, n_entries, true, lut != NULL };
    memset(tlut_obj, 0, sizeof(*tlut_obj));
    memcpy(tlut_obj, &t, sizeof(t));
}

void GXLoadTlut(GXTlutObj* obj, u32 idx)
{
    if (idx < GXB_MAX_TLUTS) {
        state()->tlut[idx] = *(const GxbTlut*)obj;
    }
}

void pc_gx_tlut_set_native_le(unsigned int idx)
{
    if (idx < GXB_MAX_TLUTS) {
        state()->tlut[idx].big_endian = false;
    }
}

void GXInvalidateTexAll(void) {}

void GXSetZTexture(GXZTexOp op, GXTexFmt fmt, u32 bias)
{
    (void)op;
    (void)fmt;
    (void)bias;
}

void GXSetBlendMode(GXBlendMode type, GXBlendFactor src_factor, GXBlendFactor dst_factor, GXLogicOp op)
{
    (void)op;
    GxbState* st = state();
    st->blend_type = (uint8_t)type;
    st->blend_src = (uint8_t)src_factor;
    st->blend_dst = (uint8_t)dst_factor;
}

void GXSetAlphaCompare(GXCompare comp0, u8 ref0, GXAlphaOp op, GXCompare comp1, u8 ref1)
{
    GxbState* st = state();
    st->alpha_comp0 = (uint8_t)comp0;
    st->alpha_ref0 = ref0;
    st->alpha_op = (uint8_t)op;
    st->alpha_comp1 = (uint8_t)comp1;
    st->alpha_ref1 = ref1;
}

void GXSetZMode(GXBool compare_enable, GXCompare func, GXBool update_enable)
{
    GxbState* st = state();
    st->z_enable = compare_enable != 0;
    st->z_func = (uint8_t)func;
    st->z_update = update_enable != 0;
}

void GXSetColorUpdate(GXBool update_enable)
{
    state()->color_update = update_enable != 0;
}

void GXSetAlphaUpdate(GXBool update_enable)
{
    state()->alpha_update = update_enable != 0;
}

void GXSetCullMode(GXCullMode mode)
{
    state()->cull_mode = (uint8_t)mode;
}

void GXSetFog(GXFogType type, f32 startz, f32 endz, f32 nearz, f32 farz, GXColor color)
{
    GxbState* st = state();
    st->fog_type = (uint8_t)type;
    st->fog_start = startz;
    st->fog_end = endz;
    st->fog_near = nearz;
    st->fog_far = farz;
    st->fog_color = gxb_rgba(color.r, color.g, color.b, color.a);
}

void GXSetFogRangeAdj(GXBool enable, u16 center, GXFogAdjTable* table)
{
    (void)enable;
    (void)center;
    (void)table;
}

void GXSetCoPlanar(GXBool enable) { (void)enable; }
void GXSetDither(GXBool dither) { (void)dither; }
void GXSetDstAlpha(GXBool enable, u8 alpha) { (void)enable; (void)alpha; }
void GXSetFieldMask(GXBool odd_mask, GXBool even_mask) { (void)odd_mask; (void)even_mask; }
void GXSetFieldMode(GXBool field_mode, GXBool half_aspect_ratio) { (void)field_mode; (void)half_aspect_ratio; }
void GXSetPixelFmt(GXPixelFmt pix_fmt, GXZFmt16 z_fmt) { (void)pix_fmt; (void)z_fmt; }
void GXSetZCompLoc(GXBool before_tex) { (void)before_tex; }
void GXSetLineWidth(u8 width, GXTexOffset texOffsets) { (void)width; (void)texOffsets; }
void GXSetPointSize(u8 pointSize, GXTexOffset texOffsets) { (void)pointSize; (void)texOffsets; }
void GXEnableTexOffsets(GXTexCoordID coord, GXBool line_enable, GXBool point_enable)
{
    (void)coord;
    (void)line_enable;
    (void)point_enable;
}
OSThread* GXSetCurrentGXThread(void)
{
    return NULL;
}
