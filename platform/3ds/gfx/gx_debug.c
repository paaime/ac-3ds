#include "gx_debug.h"

#include <stdio.h>

#define POS_MTX_STRIDE 3 /* GX_PNMTXn ids step by 3 */

static bool s_armed;
static bool s_active;
static uint32_t s_draw_index;

void gxd_arm(void)
{
    s_armed = true;
}

void gxd_frame_begin(void)
{
    s_active = s_armed;
    s_armed = false;
    s_draw_index = 0;
    if (s_active) {
        fprintf(stderr, "[gxdump] frame begin\n");
    }
}

void gxd_frame_end(void)
{
    if (s_active) {
        fprintf(stderr, "[gxdump] frame end: %lu draws\n", (unsigned long)s_draw_index);
    }
    s_active = false;
}

bool gxd_active(void)
{
    return s_active;
}

static void project(const GxbState* st, const float* p, float* ndc)
{
    uint32_t m = st->current_mtx / POS_MTX_STRIDE;
    const GxbMtx34* mv = &st->pos_mtx[m < GXB_POS_MATRICES ? m : 0];
    float eye[4];
    for (int r = 0; r < 3; r++) {
        eye[r] = mv->m[r][0] * p[0] + mv->m[r][1] * p[1] + mv->m[r][2] * p[2] + mv->m[r][3];
    }
    eye[3] = 1.0f;
    float clip[4];
    for (int r = 0; r < 4; r++) {
        clip[r] = st->projection[r][0] * eye[0] + st->projection[r][1] * eye[1] + st->projection[r][2] * eye[2] +
                  st->projection[r][3] * eye[3];
    }
    float w = clip[3] != 0.0f ? clip[3] : 1.0f;
    ndc[0] = clip[0] / w;
    ndc[1] = clip[1] / w;
    ndc[2] = clip[2] / w;
}

static void log_texmap(const GxbState* st, int map)
{
    if (map == GX_TEV_NONE || map >= GXB_MAX_TEXMAPS) {
        return;
    }
    const GxbTexObj* t = &st->texmap[map];
    fprintf(stderr, " map%d[fmt %u %ux%u%s wrap %u/%u %p]", map, t->format, t->width, t->height,
            t->is_ci ? " ci" : "", t->wrap_s, t->wrap_t, t->data);
}

static void log_stage(const GxTevStage* s, int i)
{
    const GxTevChannel* c = &s->color;
    const GxTevChannel* a = &s->alpha;
    fprintf(stderr, " s%d{c %u,%u,%u,%u op%u b%u s%u ->%u | a %u,%u,%u,%u op%u ->%u | tc%d map%d k%u/%u}", i,
            c->in[0], c->in[1], c->in[2], c->in[3], c->op, c->bias, c->scale, c->out_reg, a->in[0], a->in[1],
            a->in[2], a->in[3], a->op, a->out_reg, s->tex_coord == GX_TEV_NONE ? -1 : s->tex_coord,
            s->tex_map == GX_TEV_NONE ? -1 : s->tex_map, s->kcolor_sel, s->kalpha_sel);
}

void gxd_draw(const GxbState* st, const GxdDraw* d)
{
    if (!s_active) {
        return;
    }
    float ndc[3] = { 0.0f, 0.0f, 0.0f };
    if (d->first_pos != NULL) {
        project(st, d->first_pos, ndc);
    }
    const uint8_t* col = d->first_color;
    fprintf(stderr, "[gxdump] #%lu %s prim %02X n %lu ndc %.2f,%.2f,%.2f col %02X%02X%02X%02X pica %d",
            (unsigned long)s_draw_index++, d->fate, d->primitive, (unsigned long)d->count, ndc[0], ndc[1], ndc[2],
            col ? col[0] : 0, col ? col[1] : 0, col ? col[2] : 0, col ? col[3] : 0, d->pica_stages);
    fprintf(stderr, " blend %u %u/%u cu%d au%d z %d/%u/%d at %u:%u op%u %u:%u cull %u proj %u vtx n%d c%d t%d",
            st->blend_type, st->blend_src, st->blend_dst, st->color_update, st->alpha_update, st->z_enable,
            st->z_func, st->z_update, st->alpha_comp0, st->alpha_ref0, st->alpha_op, st->alpha_comp1,
            st->alpha_ref1, st->cull_mode, st->projection_type, st->has_normal, st->has_color, st->has_texcoord);
    fprintf(stderr, " vp %.0f,%.0f,%.0f,%.0f sc %lu,%lu,%lu,%lu", st->viewport[0], st->viewport[1], st->viewport[2],
            st->viewport[3], (unsigned long)st->scissor[0], (unsigned long)st->scissor[1],
            (unsigned long)st->scissor[2], (unsigned long)st->scissor[3]);
    fprintf(stderr, " fog %u %.0f-%.0f (%.0f-%.0f) %08lX", st->fog_type, st->fog_start, st->fog_end, st->fog_near,
            st->fog_far, (unsigned long)st->fog_color);
    fprintf(stderr, " regs %08lX %08lX %08lX %08lX k %08lX", (unsigned long)st->tev.reg[0],
            (unsigned long)st->tev.reg[1], (unsigned long)st->tev.reg[2], (unsigned long)st->tev.reg[3],
            (unsigned long)st->tev.konst[0]);
    for (int i = 0; i < st->tev.stage_count && i < GX_TEV_MAX_STAGES; i++) {
        log_stage(&st->tev.stages[i], i);
        log_texmap(st, st->tev.stages[i].tex_map);
    }
    fputc('\n', stderr);
}
