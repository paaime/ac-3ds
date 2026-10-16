#include "gx_render.h"

#include <3ds.h>
#include <citro3d.h>
#include <stdio.h>
#include <string.h>

#include "gx_bind.h"
#include "gx_debug.h"
#include "gx_efb.h"
#include "gx_fog_apply.h"
#include "gx_gpu_check.h"
#include "gx_pixel.h"
#include "gx_screen.h"
#include "gx_shbin.h"
#include "gx_tev_cache.h"
#include "gx_texcache.h"
#include "gx_uniforms.h"

_Static_assert((int)PICA_SRC_PRIMARY == (int)GPU_PRIMARY_COLOR, "TEVSRC");
_Static_assert((int)PICA_SRC_TEXTURE0 == (int)GPU_TEXTURE0, "TEVSRC");
_Static_assert((int)PICA_SRC_PREVIOUS_BUFFER == (int)GPU_PREVIOUS_BUFFER, "TEVSRC");
_Static_assert((int)PICA_SRC_CONSTANT == (int)GPU_CONSTANT, "TEVSRC");
_Static_assert((int)PICA_SRC_PREVIOUS == (int)GPU_PREVIOUS, "TEVSRC");
_Static_assert((int)PICA_OP_RGB_ONE_MINUS_ALPHA == (int)GPU_TEVOP_RGB_ONE_MINUS_SRC_ALPHA, "TEVOP");
_Static_assert((int)PICA_OP_A_ONE_MINUS_ALPHA == (int)GPU_TEVOP_A_ONE_MINUS_SRC_ALPHA, "TEVOP");
_Static_assert((int)PICA_FUNC_MULTIPLY_ADD == (int)GPU_MULTIPLY_ADD, "COMBINEFUNC");
_Static_assert((int)PICA_SCALE_4 == (int)GPU_TEVSCALE_4, "TEVSCALE");

typedef struct {
    float pos[3];
    float nrm[3];
    u8 color[4];
    float tc[2];
} GxrVertex;

#define RING_VERTICES   65536
#define GX_PRIM_QUADS   0x80
#define GX_PRIM_TRIS    0x90
#define GX_PRIM_STRIP   0x98
#define GX_PRIM_FAN     0xA0
#define SHADER_TEXGENS  GXBIND_TEXGEN_UNITS
#define FALLBACK_SIDE   8 /* smallest PICA texture */
/* Room for one draw (~250 words) and the frame's end: citro3d svcBreaks on command buffer overflow. */
#define DRAW_CMD_RESERVE_WORDS 1024

static struct {
    DVLB_s* dvlb;
    shaderProgram_s program;
    bool program_ready;
    C3D_AttrInfo attr;
    C3D_BufInfo buf;
    GxrVertex* ring;
    u32 ring_used;
    C3D_Tex fallback_tex;
    bool fallback_ready;
    C3D_Tex black_tex;
    bool black_ready;
    bool batch_open;
    u8 primitive;
    u32 batch_start;
    GxrVertex current;
    bool have_current;
    GxrVertex quad[4];
    int quad_count;
    GxrStats stats;
    GxTevCache tev_cache;
    uint64_t tev_stamp;
    bool tev_ok;
    PicaCombiner tev_combiner;
    const char* tev_reason;
    GxScreenRect viewport;
    GxScreenRect scissor;
    bool view_valid;
    GxScreenTarget target;
    float wide_factor; /* below 1 while the top screen shows Wide */
    GxViewportFold fold;
    bool view_visible;
    PicaCombiner combiner;
    bool combiner_valid;
    bool in_frame;
} R;

static void check_draw(void);

static GxrPictureShape s_picture_shape; /* outside R, which gxr_init clears */

void gxr_set_sharp_textures(bool on)
{
    gxbind_set_sharp(on);
}

void gxr_set_picture_shape(GxrPictureShape shape)
{
    s_picture_shape = shape;
}

static void emit(const GxrVertex* v)
{
    if (R.ring_used >= RING_VERTICES) {
        R.stats.vertex_overflow++;
        return;
    }
    R.ring[R.ring_used++] = *v;
}

static void commit_current(void)
{
    if (!R.have_current) {
        return;
    }
    R.have_current = false;
    if (R.primitive != GX_PRIM_QUADS) {
        emit(&R.current);
        return;
    }
    R.quad[R.quad_count++] = R.current;
    if (R.quad_count == 4) {
        emit(&R.quad[0]);
        emit(&R.quad[1]);
        emit(&R.quad[2]);
        emit(&R.quad[0]);
        emit(&R.quad[2]);
        emit(&R.quad[3]);
        R.quad_count = 0;
    }
}

void gxr_begin(uint8_t primitive, uint16_t vertex_count)
{
    (void)vertex_count;
    R.batch_open = true;
    R.primitive = primitive;
    R.batch_start = R.ring_used;
    R.have_current = false;
    R.quad_count = 0;
    memset(&R.current, 0, sizeof(R.current));
    R.current.nrm[2] = 1.0f;
    memset(R.current.color, 0xFF, sizeof(R.current.color));
}

void gxr_position(float x, float y, float z)
{
    commit_current();
    R.current.pos[0] = x;
    R.current.pos[1] = y;
    R.current.pos[2] = z;
    R.have_current = true;
}

void gxr_normal(float x, float y, float z)
{
    R.current.nrm[0] = x;
    R.current.nrm[1] = y;
    R.current.nrm[2] = z;
}

void gxr_color(uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
    R.current.color[0] = r;
    R.current.color[1] = g;
    R.current.color[2] = b;
    R.current.color[3] = a;
}

void gxr_texcoord(float s, float t)
{
    R.current.tc[0] = s;
    R.current.tc[1] = t;
}

static u32 to_texenv_color(uint32_t rgba)
{
    return ((rgba >> 24) & 0xFF) | (((rgba >> 16) & 0xFF) << 8) | (((rgba >> 8) & 0xFF) << 16) |
           ((rgba & 0xFF) << 24);
}

static bool same_combiner(const PicaCombiner* a, const PicaCombiner* b)
{
    if (a->stage_count != b->stage_count || a->uses_buffer_color != b->uses_buffer_color ||
        (a->uses_buffer_color && a->buffer_color != b->buffer_color)) {
        return false;
    }
    for (int i = 0; i < a->stage_count; i++) {
        const PicaCombinerStage* x = &a->stages[i];
        const PicaCombinerStage* y = &b->stages[i];
        if (memcmp(&x->rgb, &y->rgb, sizeof(x->rgb)) != 0 || memcmp(&x->alpha, &y->alpha, sizeof(x->alpha)) != 0 ||
            x->constant != y->constant) {
            return false;
        }
    }
    return true;
}

/* C3D_GetTexEnv marks a stage for re-sending whether it changes or not. */
static void apply_combiner(const PicaCombiner* p)
{
    if (R.combiner_valid && same_combiner(p, &R.combiner)) {
        return;
    }
    R.combiner = *p;
    R.combiner_valid = true;
    for (int i = 0; i < p->stage_count; i++) {
        const PicaCombinerStage* s = &p->stages[i];
        C3D_TexEnv* env = C3D_GetTexEnv(i);
        C3D_TexEnvInit(env);
        C3D_TexEnvSrc(env, C3D_RGB, s->rgb.src[0], s->rgb.src[1], s->rgb.src[2]);
        C3D_TexEnvOpRgb(env, s->rgb.op[0], s->rgb.op[1], s->rgb.op[2]);
        C3D_TexEnvFunc(env, C3D_RGB, s->rgb.func);
        C3D_TexEnvScale(env, C3D_RGB, s->rgb.scale);
        C3D_TexEnvSrc(env, C3D_Alpha, s->alpha.src[0], s->alpha.src[1], s->alpha.src[2]);
        C3D_TexEnvOpAlpha(env, s->alpha.op[0], s->alpha.op[1], s->alpha.op[2]);
        C3D_TexEnvFunc(env, C3D_Alpha, s->alpha.func);
        C3D_TexEnvScale(env, C3D_Alpha, s->alpha.scale);
        C3D_TexEnvColor(env, to_texenv_color(s->constant));
    }
    for (int i = p->stage_count; i < PICA_COMBINER_MAX_STAGES; i++) {
        C3D_TexEnvInit(C3D_GetTexEnv(i));
    }
    if (p->uses_buffer_color) {
        C3D_TexEnvBufColor(to_texenv_color(p->buffer_color));
    }
}

static void fallback_combiner(const GxbState* st, PicaCombiner* p)
{
    memset(p, 0, sizeof(*p));
    memset(p->unit_texmap, GX_TEV_NONE, sizeof(p->unit_texmap));
    p->stage_count = 1;
    PicaCombinerStage* s = &p->stages[0];
    uint8_t map = st->tev.stages[0].tex_map;
    bool textured = map != GX_TEV_NONE;
    if (textured) {
        p->unit_texmap[0] = map;
    }
    uint8_t func = textured ? PICA_FUNC_MODULATE : PICA_FUNC_REPLACE;
    uint8_t src0 = textured ? PICA_SRC_TEXTURE0 : PICA_SRC_PRIMARY;
    s->rgb = (PicaCombinerOp){ func, { src0, PICA_SRC_PRIMARY, PICA_SRC_PRIMARY }, { 0, 0, 0 }, 0 };
    s->alpha = (PicaCombinerOp){ func, { src0, PICA_SRC_PRIMARY, PICA_SRC_PRIMARY }, { 0, 0, 0 }, 0 };
}

static bool same_rect(GxScreenRect a, GxScreenRect b)
{
    return a.left == b.left && a.top == b.top && a.width == b.width && a.height == b.height;
}

/* GX draws nothing outside its viewport: the scissor clips to both; the PICA viewport stays the whole target. */
static bool apply_view(const GxbState* st)
{
    GxScreenRect vp = { st->viewport[0], st->viewport[1], st->viewport[2], st->viewport[3] };
    if (st->viewport_mark == GX_WIDE_UI) {
        vp = gx_screen_ui_viewport(vp, R.wide_factor, GXB_EFB_WIDTH, GXB_EFB_HEIGHT);
    }
    GxScreenRect sc = { (float)st->scissor[0], (float)st->scissor[1], (float)st->scissor[2],
                        (float)st->scissor[3] };
    if (R.view_valid && same_rect(vp, R.viewport) && same_rect(sc, R.scissor)) {
        return R.view_visible;
    }
    R.viewport = vp;
    R.scissor = sc;
    R.view_valid = true;
    R.fold = gx_screen_viewport_fold(vp, R.target);
    GxScreenRect visible = gx_screen_intersect(vp, sc);
    PicaRect c = gx_screen_to_pica(visible, R.target);
    R.view_visible = c.w != 0 && c.h != 0;
    if (!R.view_visible) {
        return false;
    }
    if (gx_screen_is_full(visible, R.target)) {
        C3D_SetScissor(GPU_SCISSOR_DISABLE, 0, 0, 0, 0);
    } else {
        C3D_SetScissor(GPU_SCISSOR_NORMAL, c.x, c.y, c.x + c.w, c.y + c.h);
    }
    return true;
}

static GPU_Primitive_t pica_primitive(uint8_t gx, bool* supported)
{
    *supported = true;
    switch (gx) {
    case GX_PRIM_QUADS:
    case GX_PRIM_TRIS: return GPU_TRIANGLES;
    case GX_PRIM_STRIP: return GPU_TRIANGLE_STRIP;
    case GX_PRIM_FAN: return GPU_TRIANGLE_FAN;
    default: *supported = false; return GPU_TRIANGLES;
    }
}

static bool cmdbuf_has_room(void)
{
    return gpuCmdBuf != NULL && gpuCmdBufSize - gpuCmdBufOffset >= DRAW_CMD_RESERVE_WORDS;
}

bool gxr_batch_open(void)
{
    return R.batch_open;
}

static void reset_batch(void)
{
    R.batch_open = false;
    R.batch_start = R.ring_used;
    R.have_current = false;
    R.quad_count = 0;
}

static void debug_draw(const GxbState* st, u32 count, int pica_stages, const char* fate)
{
    if (!gxd_active()) {
        return;
    }
    const GxrVertex* first = count > 0 ? &R.ring[R.batch_start] : NULL;
    GxdDraw d = { R.primitive, count, first ? first->pos : NULL, first ? first->color : NULL, pica_stages, fate };
    gxd_draw(st, &d);
}

static const PicaCombiner* translate_tev(const GxbState* st, const char** reason)
{
    if (st->stamps.tev == 0 || st->stamps.tev != R.tev_stamp) {
        R.tev_reason = NULL;
        const PicaCombiner* p = gx_tev_cache_get(&R.tev_cache, &st->tev, gx_tev_translate, &R.tev_reason);
        R.tev_stamp = st->stamps.tev;
        R.tev_ok = p != NULL;
        if (R.tev_ok) {
            R.tev_combiner = *p;
        }
    }
    *reason = R.tev_reason;
    return R.tev_ok ? &R.tev_combiner : NULL;
}

void gxr_end(const GxbState* st)
{
    /* emu64 (TARGET_PC) calls GXEnd again after batches that already ended: ignored, as in pc_gx.c. */
    if (!R.batch_open || R.ring_used < R.batch_start) {
        reset_batch();
        return;
    }
    commit_current();
    R.batch_open = false;
    u32 count = R.ring_used - R.batch_start;
    bool supported;
    GPU_Primitive_t prim = pica_primitive(R.primitive, &supported);
    if (!supported) {
        debug_draw(st, count, -1, "skipped");
        R.ring_used = R.batch_start;
        R.stats.skipped_primitives++;
        return;
    }
    if (count == 0 || !gxp_apply_cull(st)) {
        debug_draw(st, count, -1, "culled");
        R.ring_used = R.batch_start;
        return;
    }
    if (!apply_view(st)) {
        debug_draw(st, count, -1, "offscreen");
        R.ring_used = R.batch_start;
        return;
    }
    if (!cmdbuf_has_room()) {
        debug_draw(st, count, -1, "dropped");
        R.ring_used = R.batch_start;
        R.stats.dropped_draws++;
        return;
    }
    u32 cmd_start = gpuCmdBufOffset;

    PicaCombiner combiner;
    const char* reason = NULL;
    const PicaCombiner* translated = translate_tev(st, &reason);
    if (translated != NULL) {
        combiner = *translated;
    } else {
        R.stats.tev_fallbacks++;
        fallback_combiner(st, &combiner);
    }
    GxUvAffine uv[SHADER_TEXGENS];
    GxBindCounts counts = { 0, 0 };
    gxbind_draw(st, &combiner, uv, &counts);
    R.stats.efb_copy_draws += counts.efb_copy_draws;
    R.stats.texture_failures += counts.texture_failures;
    apply_combiner(&combiner);
    GxViewportFold fold = R.fold;
    fold.widen_x = gx_screen_projection_widen((GxWideMark)st->projection_mark,
                                              st->projection_type == GXB_PROJ_ORTHOGRAPHIC, R.wide_factor);
    gxu_upload_draw(st, fold, uv);
    gxp_apply_depth(st);
    gxp_apply_blend(st);
    if (gxf_apply(st)) {
        R.stats.fog_tables++;
    }
    gxp_apply_alpha_test(st);
    gxp_mark_applied();

    debug_draw(st, count, combiner.stage_count, "draw");
    C3D_DrawArrays(prim, (int)R.batch_start, (int)count);
    R.stats.draws++;
    if (gxg_active()) {
        check_draw();
    }
    R.stats.vertices += count;
    u32 words = gpuCmdBufOffset - cmd_start;
    R.stats.cmd_words += words;
    R.stats.max_draw_cmd_words = words > R.stats.max_draw_cmd_words ? words : R.stats.max_draw_cmd_words;
}

void gxr_gpu_check(uint32_t frames)
{
    gxg_arm(frames);
}

bool gxr_gpu_sync(void)
{
    if (R.ring_used > 0) { /* the ring is otherwise flushed only at frame end */
        GSPGPU_FlushDataCache(R.ring, sizeof(GxrVertex) * R.ring_used);
    }
    C3D_FrameSplit(GX_CMDLIST_FLUSH);
    if (!gxg_run_and_wait()) {
        return false;
    }
    gxg_empty_queue();
    return true;
}

static void check_draw(void)
{
    if (!gxr_gpu_sync()) {
        gxg_draw_failed(R.stats.draws);
    }
}

bool gxr_gpu_check_frame_end(void)
{
    return gxg_frame_end(R.stats.draws);
}

#define BLIT_VERTICES   6
#define SCREEN_HALF_W   200.0f /* the top screen is 400 x 240 (landscape) */
#define SCREEN_HALF_H   120.0f
#define BLIT_DEPTH      -0.5f /* inside GX's clip depth [-1, 0]; the depth test is off */

static void blit_combiner(void)
{
    PicaCombiner p;
    memset(&p, 0, sizeof(p));
    p.stage_count = 1;
    PicaCombinerOp replace = { PICA_FUNC_REPLACE, { PICA_SRC_TEXTURE0, PICA_SRC_PRIMARY, PICA_SRC_PRIMARY },
                               { 0, 0, 0 }, PICA_SCALE_1 };
    p.stages[0].rgb = replace;
    p.stages[0].alpha = replace;
    apply_combiner(&p);
}

static void blit_vertex(float x, float y, float s, float t)
{
    GxrVertex v = { { x / SCREEN_HALF_W - 1.0f, 1.0f - y / SCREEN_HALF_H, BLIT_DEPTH },
                    { 0, 0, 1 }, { 0xFF, 0xFF, 0xFF, 0xFF }, { s, t } };
    emit(&v);
}

void gxr_blit(const void* texture, float x, float y, float w, float h, float s1, float t1)
{
    if (!R.in_frame || R.batch_open || texture == NULL || !cmdbuf_has_room() ||
        R.ring_used + BLIT_VERTICES > RING_VERTICES) {
        R.stats.dropped_draws++;
        return;
    }
    u32 start = R.ring_used;
    blit_vertex(x, y, 0, 0);
    blit_vertex(x + w, y, s1, 0);
    blit_vertex(x, y + h, 0, t1);
    blit_vertex(x + w, y, s1, 0);
    blit_vertex(x + w, y + h, s1, t1);
    blit_vertex(x, y + h, 0, t1);
    gxu_upload_blit();
    blit_combiner();
    gxbind_blit((C3D_Tex*)texture);
    C3D_DepthTest(true, GPU_ALWAYS, GPU_WRITE_COLOR);
    C3D_AlphaTest(false, GPU_ALWAYS, 0);
    C3D_AlphaBlend(GPU_BLEND_ADD, GPU_BLEND_ADD, GPU_ONE, GPU_ZERO, GPU_ONE, GPU_ZERO);
    C3D_CullFace(GPU_CULL_NONE);
    C3D_FogGasMode(GPU_NO_FOG, GPU_PLAIN_DENSITY, false);
    C3D_SetScissor(GPU_SCISSOR_DISABLE, 0, 0, 0, 0);
    C3D_DrawArrays(GPU_TRIANGLES, (int)start, BLIT_VERTICES);
    R.stats.draws++;
    R.stats.vertices += BLIT_VERTICES;
    gxr_invalidate_state();
    R.view_valid = false;
    R.batch_start = R.ring_used;
}

void gxr_draw_bars(void)
{
    GxScreenTarget t = R.target;
    float width = (float)t.fb_height; /* landscape */
    float height = (float)t.fb_width;
    float right = t.image_x + t.image_w;
    if (!R.black_ready || t.image_x <= 0.0f) {
        return;
    }
    gxr_blit(&R.black_tex, 0.0f, 0.0f, t.image_x, height, 1.0f, 1.0f);
    gxr_blit(&R.black_tex, right, 0.0f, width - right, height, 1.0f, 1.0f);
}

static bool load_program(void)
{
    R.dvlb = DVLB_ParseFile((u32*)gx_shbin, gx_shbin_size);
    if (R.dvlb == NULL || R.dvlb->numDVLE < 1 || R_FAILED(shaderProgramInit(&R.program))) {
        return false;
    }
    if (R_FAILED(shaderProgramSetVsh(&R.program, &R.dvlb->DVLE[0]))) {
        shaderProgramFree(&R.program);
        return false;
    }
    R.program_ready = true;
    return gxu_locate(R.program.vertexShader);
}

static bool create_solid_texture(C3D_Tex* tex, uint32_t rgba)
{
    if (!C3D_TexInit(tex, FALLBACK_SIDE, FALLBACK_SIDE, GPU_RGBA8)) {
        return false;
    }
    uint32_t* texels = tex->data;
    for (size_t i = 0; i < tex->size / sizeof(uint32_t); i++) {
        texels[i] = rgba;
    }
    C3D_TexFlush(tex);
    return true;
}

static bool create_fallback_texture(void)
{
    R.fallback_ready = create_solid_texture(&R.fallback_tex, 0xFFFFFFFFu);
    gxbind_set_fallback(R.fallback_ready ? &R.fallback_tex : NULL);
    R.black_ready = R.fallback_ready && create_solid_texture(&R.black_tex, 0x000000FFu);
    if (R.fallback_ready && !R.black_ready) {
        fprintf(stderr, "[gx] no memory for the 4:3 bars' texture; the bars show the clear colour\n");
    }
    return R.fallback_ready;
}

bool gxr_init(void)
{
    memset(&R, 0, sizeof(R));
    gxg_arm(0);
    gxf_invalidate();
    R.ring = linearAlloc(sizeof(GxrVertex) * RING_VERTICES);
    if (R.ring == NULL || !load_program() || !gxtc_init() || !create_fallback_texture()) {
        gxr_shutdown();
        return false;
    }
    AttrInfo_Init(&R.attr);
    AttrInfo_AddLoader(&R.attr, 0, GPU_FLOAT, 3);
    AttrInfo_AddLoader(&R.attr, 1, GPU_FLOAT, 3);
    AttrInfo_AddLoader(&R.attr, 2, GPU_UNSIGNED_BYTE, 4);
    AttrInfo_AddLoader(&R.attr, 3, GPU_FLOAT, 2);
    BufInfo_Init(&R.buf);
    BufInfo_Add(&R.buf, R.ring, sizeof(GxrVertex), 4, 0x3210);
    return true;
}

void gxr_shutdown(void)
{
    gxe_shutdown();
    gxtc_shutdown();
    if (R.black_ready) {
        C3D_TexDelete(&R.black_tex);
        R.black_ready = false;
    }
    gxbind_set_fallback(NULL);
    gxbind_invalidate();
    if (R.fallback_ready) {
        C3D_TexDelete(&R.fallback_tex);
        R.fallback_ready = false;
    }
    if (R.program_ready) {
        shaderProgramFree(&R.program);
        R.program_ready = false;
    }
    if (R.dvlb != NULL) {
        DVLB_Free(R.dvlb);
        R.dvlb = NULL;
    }
    if (R.ring != NULL) {
        linearFree(R.ring);
        R.ring = NULL;
    }
}

void gxr_frame_begin(float target_width, float target_height)
{
    R.target = gx_screen_target(GXB_EFB_WIDTH, GXB_EFB_HEIGHT, (uint32_t)target_width, (uint32_t)target_height,
                                s_picture_shape == GXR_PICTURE_4_3);
    R.wide_factor = s_picture_shape == GXR_PICTURE_WIDE ? gx_screen_wide_factor(R.target) : 1.0f;
    gxe_set_image_area(R.target.image_x / target_width, R.target.image_y / target_height,
                       R.target.image_w / target_width, R.target.image_h / target_height);
    R.in_frame = true;
    R.view_valid = false; /* C3D_FrameDrawOn reset the viewport and scissor */
    gxr_invalidate_state();
    memset(&R.stats, 0, sizeof(R.stats));
    if (gxg_active()) {
        gxd_arm();
    }
    gxd_frame_begin();
    gxe_frame_begin();
    R.ring_used = 0;
    reset_batch();
    gxtc_frame_begin();
    C3D_BindProgram(&R.program);
    C3D_SetAttrInfo(&R.attr);
    C3D_SetBufInfo(&R.buf);
    C3D_TexEnvBufUpdate(C3D_Both, 0); /* the combiner buffer only holds a constant */
}

void gxr_invalidate_state(void)
{
    gxu_invalidate();
    R.combiner_valid = false;
    gxbind_invalidate();
    gxf_invalidate();
    gxp_invalidate();
}

void gxr_set_target(float target_width, float target_height)
{
    R.target = gx_screen_target(GXB_EFB_WIDTH, GXB_EFB_HEIGHT, (uint32_t)target_width, (uint32_t)target_height, false);
    R.wide_factor = 1.0f;
    R.view_valid = false; /* C3D_FrameDrawOn reset the viewport and scissor */
    gxr_invalidate_state();
    gxtc_pass_begin();
}

bool gxr_in_frame(void)
{
    return R.in_frame;
}

void gxr_frame_end(void)
{
    R.in_frame = false;
    gxd_frame_end();
    if (R.ring_used > 0) {
        GSPGPU_FlushDataCache(R.ring, sizeof(GxrVertex) * R.ring_used);
    }
}

#define DEFAULT_CLEAR_COLOR 0x00000000u

static uint32_t s_clear_color = DEFAULT_CLEAR_COLOR;

void gxr_set_clear_color(uint32_t rgba)
{
    s_clear_color = rgba;
}

uint32_t gxr_clear_color(void)
{
    return s_clear_color;
}

GxrStats gxr_stats(void)
{
    GxrStats s = R.stats;
    s.texture_uploads = gxtc_uploads_this_frame();
    s.texture_entries = gxtc_entry_count();
    s.texture_bytes = gxtc_bytes();
    s.texture_ticks = gxtc_upload_ticks_this_frame();
    return s;
}
