#include <proto/graphics.h>

#include "screen.h"
#include "conio.h"
#include "gfx_render.h"
#include "shapes.h"
#include "vector_outline.h"
#include "data.h"

/* ============================================================================
 * HAND-CRAFTED SHAPE VECTORS
 *
 * Shapes listed in s_hand_shapes[] are drawn from these hand-authored
 * vertex loops instead of the auto-traced cell bitmap. Design away:
 *
 *   - Coordinates are lattice corners: integers 0..width, x rightward,
 *     y downward. The cell at column c, row r occupies the square
 *     (c,r)..(c+1,r+1). A 5x5 shape therefore spans 0..5.
 *   - Each loop is a closed polygon: the last vertex connects back to
 *     the first automatically. Pen down at loops[0].pts[0], line to
 *     every other vertex, close.
 *   - Holes are just inner loops - everything is stroked, nothing is
 *     filled, so winding direction does not matter.
 *   - Loop coordinates scale automatically: x by SCREEN_PIXEL_WIDTH/40,
 *     y by the runtime screen height/24. Author in the shape's own
 *     0..width grid and it lands correctly on PAL and NTSC.
 *
 * To design a shape: draw it on grid paper (0..width), walk the outline
 * clockwise, write down each corner, add inner loops for holes. Add an
 * entry to s_hand_shapes[] with your shape id. Shapes not in the table
 * fall back to the auto-traced bitmap outlines.
 * ========================================================================== */

/* Example: 5x5 plus - arms one cell thick through the centre. */
static const vo_point s_plus5_loop[] = {
    {2, 0}, {3, 0}, {3, 2}, {5, 2}, {5, 3}, {3, 3},
    {3, 5}, {2, 5}, {2, 3}, {0, 3}, {0, 2}, {2, 2}
};

/* Example: 5x5 hollow square - outer loop + inner hole loop. */
static const vo_point s_hollow5_outer[] = {
    {0, 0}, {5, 0}, {5, 5}, {0, 5}
};
static const vo_point s_hollow5_hole[] = {
    {2, 2}, {3, 2}, {3, 3}, {2, 3}
};

typedef struct {
    uint8_t len;
    const vo_point *pts;
} hand_loop;

typedef struct {
    uint8_t shape_id;
    uint8_t loop_count;
    const hand_loop *loops;
} hand_shape;

/* Key entries by the server's shape id (0..18). Adjust the ids below to
 * target the shapes you want to override. */
static const hand_shape s_hand_shapes[] = {
    /* shape 1: 5x5 plus */
    { 1, 1, (const hand_loop[]) { { 12, s_plus5_loop } } },
    /* shape 2: 5x5 hollow square */
    { 2, 2, (const hand_loop[]) { { 4, s_hollow5_outer }, { 4, s_hollow5_hole } } },
};

static const hand_shape *hand_vector_for(uint8_t shape_id)
{
    uint8_t i;

    for (i = 0; i < (uint8_t)(sizeof(s_hand_shapes) / sizeof(s_hand_shapes[0])); i++) {
        if (s_hand_shapes[i].shape_id == shape_id) {
            return &s_hand_shapes[i];
        }
    }
    return NULL;
}

/* Line-art vector renderer (Asteroids/Elite style): every traced contour
 * is drawn as a closed pixel-line path - outers and holes alike. No area
 * fill, no TmpRas, no fill-convention artifacts. */

static void draw_contour_lines(struct RastPort *rp, const vo_point *pts,
                               uint8_t len)
{
    uint8_t i;

    if (len < 2) {
        return;
    }
    Move(rp, (LONG)pts[0].x, (LONG)pts[0].y);
    for (i = 1; i < len; i++) {
        Draw(rp, (LONG)pts[i].x, (LONG)pts[i].y);
    }
    Draw(rp, (LONG)pts[0].x, (LONG)pts[0].y); /* close the loop */
}

static void draw_vector(struct RastPort *rp, const vo_outline *ol,
                        int32_t base_x, int32_t base_y,
                        int32_t scale_x, int32_t height_px)
{
    static vo_point px_pts[VO_MAX_PTS]; /* static: off the 64KB stack */
    uint8_t c, i, n;

    SetAPen(rp, 2); /* reserved shape pen */

    for (c = 0; c < ol->contour_count; c++) {
        n = ol->contour_len[c];
        for (i = 0; i < n; i++) {
            const vo_point *v = &ol->pts[ol->contour_start[c] + i];
            int32_t px = base_x + (int32_t)v->x * scale_x;
            int32_t py = base_y + (int32_t)v->y * height_px / REG_WORLD_HEIGHT;

            /* Bodies can transiently sit outside the registered screen
             * while wrapping; clamp so lines stay on the bitmap. */
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
        draw_contour_lines(rp, px_pts, n);
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

    /* Hand-crafted override: authored loops win over the auto-traced
     * bitmap outline. Same scaling and stroking as traced contours. */
    {
        const hand_shape *hand = hand_vector_for(shape_id);

        if (hand != NULL) {
            SetAPen(rp, 2);
            for (uint8_t c = 0; c < hand->loop_count; c++) {
                static vo_point hand_pts[VO_MAX_PTS];
                const hand_loop *loop = &hand->loops[c];
                int32_t scale_x = SCREEN_PIXEL_WIDTH / REG_WORLD_WIDTH;
                uint8_t i;

                for (i = 0; i < loop->len; i++) {
                    int32_t px = base_x + (int32_t)loop->pts[i].x * scale_x;
                    int32_t py = base_y +
                        (int32_t)loop->pts[i].y * (int32_t)h / REG_WORLD_HEIGHT;

                    if (px < 0) {
                        px = 0;
                    }
                    if (px > SCREEN_PIXEL_WIDTH - 1) {
                        px = SCREEN_PIXEL_WIDTH - 1;
                    }
                    if (py < 0) {
                        py = 0;
                    }
                    if (py > (int32_t)h - 1) {
                        py = (int32_t)h - 1;
                    }
                    hand_pts[i].x = (int16_t)px;
                    hand_pts[i].y = (int16_t)py;
                }
                draw_contour_lines(rp, hand_pts, loop->len);
            }
            return;
        }
    }

    /* Vector mode: trace the embedded cell bitmap into closed contours and
     * stroke them as pixel lines. Fully off-screen shapes are skipped;
     * partial ones rely on the raster clip like the block path does. */
    if (vo_trace(shape->shape_data, width, &outline) != 0 ||
        outline.contour_count == 0) {
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
