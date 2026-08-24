#include <proto/graphics.h>

#include "screen.h"
#include "conio.h"
#include "gfx_render.h"

#include "shapes.h"

void gfx_show_shape_px(uint8_t shape_id, int16_t center_x, int16_t center_y)
{
    struct RastPort *rp;
    int32_t half_w, half_h, x0, y0, x1, y1;
    uint8_t width;

    if (shape_id >= shape_count) {
        return;
    }
    rp = (struct RastPort *)amiga_conio_draw_rp();
    if (!rp) {
        return;
    }

    width = shapes[shape_id].shape_width;

    /* Shapes are proportional to the logical 40x24 world grid; scale the
     * world-unit footprint into registered screen pixels. Pen 1: the
     * custom-screen palette is not customized yet (story 2 owns colour). */
    half_w = ((int32_t)width * SCREEN_PIXEL_WIDTH / REG_WORLD_WIDTH) / 2;
    half_h = ((int32_t)width * SCREEN_PIXEL_HEIGHT / REG_WORLD_HEIGHT) / 2;

    x0 = (int32_t)center_x - half_w;
    y0 = (int32_t)center_y - half_h;
    x1 = (int32_t)center_x + half_w - 1;
    y1 = (int32_t)center_y + half_h - 1;

    if (x0 < 0) {
        x0 = 0;
    }
    if (y0 < 0) {
        y0 = 0;
    }
    if (x1 >= SCREEN_PIXEL_WIDTH) {
        x1 = SCREEN_PIXEL_WIDTH - 1;
    }
    if (y1 >= (int32_t)amiga_conio_height()) {
        y1 = (int32_t)amiga_conio_height() - 1;
    }

    SetAPen(rp, 1);
    RectFill(rp, (LONG)x0, (LONG)y0, (LONG)x1, (LONG)y1);
}
