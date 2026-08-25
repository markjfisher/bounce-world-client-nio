#include <proto/graphics.h>
#include <graphics/gfx.h>

#include "screen.h"
#include "conio.h"
#include "gfx_render.h"
#include "shapes.h"
#include "data.h"
#include "vector_outline.h"

/* Area-fill working storage: sized once from VO_MAX_PTS (boundary edges
 * of a width-5 shape are bounded well below it), allocated statically,
 * one slot per double-buffer rastport, initialised on first use - never
 * per frame, never on the application stack. */
#define AREA_BUF_WORDS ((VO_MAX_PTS) * 5)
static struct AreaInfo s_area_info[2];
static SHORT s_area_buf[2][AREA_BUF_WORDS];
static void *s_area_rp[2] = { NULL, NULL };

static void ensure_area_init(struct RastPort *rp)
{
    int i;

    for (i = 0; i < 2; i++) {
        if (s_area_rp[i] == (void *)rp) {
            return;
        }
    }
    /* Two double-buffer rastports exist by construction; if a third ever
     * appears, evict slot 0 and rebind rather than leaving AreaInfo NULL. */
    for (i = 0; i < 2; i++) {
        if (!s_area_rp[i]) {
            break;
        }
    }
    if (i == 2) {
        i = 0;
        s_area_rp[1] = NULL;
    }
    {
        struct AreaInfo *ai = &s_area_info[i];

        /* This NDK's inline InitArea binds (areaInfo, buffer, max);
         * the rastport link is ours to make. */
        InitArea(ai, &s_area_buf[i][0], VO_MAX_PTS);
        rp->AreaInfo = ai;
        s_area_rp[i] = (void *)rp;
    }
}

static void draw_contour(struct RastPort *rp, const vo_point *pts, uint8_t len)
{
    uint8_t i;

    if (len == 0) {
        return;
    }
    AreaMove(rp, (LONG)pts[0].x, (LONG)pts[0].y);
    for (i = 1; i < len; i++) {
        AreaDraw(rp, (LONG)pts[i].x, (LONG)pts[i].y);
    }
    AreaEnd(rp);
}

/* Deterministic fill policy: every outer contour filled with the shape
 * pen first, then every enclosed hole refilled with the background pen,
 * each pass in contour order. The observable contract is that the filled
 * result equals the source cell bitmap (validated by the host rasterizer
 * test); holes nested inside other holes do not occur in the embedded
 * shapes, so two ordered passes are sufficient. */
static void draw_vector(struct RastPort *rp, const vo_outline *ol,
                        int32_t base_x, int32_t base_y,
                        int32_t scale_x, int32_t height_px)
{
    static vo_point px_pts[VO_MAX_PTS]; /* static: off the 64KB stack */
    uint8_t c, i, n;
    int pass;

    ensure_area_init(rp);

    for (pass = 0; pass < 2; pass++) {
        SetAPen(rp, pass == 0 ? 2 : 0);
        for (c = 0; c < ol->contour_count; c++) {
            if ((int)(pass == 0 ? 0 : 1) != (int)ol->is_hole[c]) {
                continue;
            }
            n = ol->contour_len[c];
            for (i = 0; i < n; i++) {
                const vo_point *v = &ol->pts[ol->contour_start[c] + i];

                {
                    /* Wire coords are int16 screen pixels, so base+extent can
                     * exceed int16 for transient off-screen bodies; clamp in
                     * int32 before narrowing so nothing wraps into view. */
                    int32_t px = base_x + (int32_t)v->x * scale_x;
                    int32_t py = base_y + (int32_t)v->y * height_px / REG_WORLD_HEIGHT;

                    if (px < 0) {
                        px = 0;
                    }
                    if (px > SCREEN_PIXEL_WIDTH - 1) {
                        px = SCREEN_PIXEL_WIDTH - 1;
                    }
                    if (py < 0) {
                        py = 0;
                    }
                    if (py > height_px - 1) {
                        py = height_px - 1;
                    }
                    px_pts[i].x = (int16_t)px;
                    px_pts[i].y = (int16_t)py;
                }
            }
            draw_contour(rp, px_pts, n);
        }
    }
}

static void draw_block(struct RastPort *rp, int32_t center_x, int32_t center_y,
                       uint8_t width)
{
    int32_t half_w, half_h, x0, y0, x1, y1;
    uint16_t h = amiga_conio_height();

    /* Shapes are proportional to the logical 40x24 world grid; scale the
     * world-unit footprint into registered screen pixels using the
     * runtime drawable height so NTSC machines stay registered. */
    half_w = ((int32_t)width * SCREEN_PIXEL_WIDTH / REG_WORLD_WIDTH) / 2;
    half_h = ((int32_t)width * h / REG_WORLD_HEIGHT) / 2;

    x0 = (int32_t)center_x - half_w;
    y0 = (int32_t)center_y - half_h;
    x1 = (int32_t)center_x + half_w - 1;
    y1 = (int32_t)center_y + half_h - 1;

    /* Bodies can transiently sit outside the registered screen while
     * wrapping. Clamp to the bitmap and drop rects that clamp away
     * entirely - a reversed RectFill (x0 > x1 or y0 > y1) is undefined
     * behaviour and crashed the guest. */
    if (x0 < 0) {
        x0 = 0;
    }
    if (y0 < 0) {
        y0 = 0;
    }
    if (x1 >= SCREEN_PIXEL_WIDTH) {
        x1 = SCREEN_PIXEL_WIDTH - 1;
    }
    if (y1 >= (int32_t)h) {
        y1 = (int32_t)h - 1;
    }
    if (x1 < x0 || y1 < y0) {
        return;
    }

    SetAPen(rp, 2); /* colour 2 is the reserved shape pen (conio palette) */
    RectFill(rp, (LONG)x0, (LONG)y0, (LONG)x1, (LONG)y1);
}

void gfx_show_shape_px(uint8_t shape_id, int16_t center_x, int16_t center_y)
{
    struct RastPort *rp;
    const ShapeRecord *shape;
    static vo_outline outline; /* static: keeps geometry off the 64KB stack */
    int32_t half_w, half_h, base_x, base_y;
    uint16_t h;
    uint8_t width;

    if (shape_id >= shape_count) {
        return;
    }
    rp = (struct RastPort *)amiga_conio_draw_rp();
    if (!rp) {
        return;
    }

    shape = &shapes[shape_id];
    width = shape->shape_width;
    if (width == 0 || !shape->shape_data) {
        return;
    }

    h = amiga_conio_height();
    half_w = ((int32_t)width * SCREEN_PIXEL_WIDTH / REG_WORLD_WIDTH) / 2;
    half_h = ((int32_t)width * h / REG_WORLD_HEIGHT) / 2;
    base_x = (int32_t)center_x - half_w;
    base_y = (int32_t)center_y - half_h;

    if (bwc_render_mode == RENDER_BLOCK) {
        draw_block(rp, center_x, center_y, width);
        return;
    }

    /* Vector mode: trace the embedded cell bitmap into closed contours.
     * If tracing ever exceeds the storage bounds, fall back to the block
     * rectangle rather than drawing partial geometry. Fully off-screen
     * shapes are skipped; partial ones rely on the raster clip like the
     * block path does. */
    if (vo_trace(shape->shape_data, width, &outline) != 0 || outline.contour_count == 0) {
        draw_block(rp, center_x, center_y, width);
        return;
    }

    if (base_x >= SCREEN_PIXEL_WIDTH || base_y >= (int32_t)h ||
        base_x + half_w < 0 || base_y + half_h < 0) {
        return;
    }

    draw_vector(rp, &outline, base_x, base_y,
                SCREEN_PIXEL_WIDTH / REG_WORLD_WIDTH, h);
}
