#include <stdint.h>
#include <string.h>

#include "gfx_shapes.h"
#include "screen.h"

uint8_t screen_buf[SCREEN_BUF_SIZE];

static uint8_t gfx_colour = GFX_COLOUR_DEFAULT;

void screen_blit_rows(uint8_t num_rows)
{
    uint8_t  y;
    uint16_t offset;
    uint8_t *dest;

    if (screen_visible == 0) {
        return;
    }

    dest = screen_visible;
    for (y = 0; y < num_rows; ++y) {
        offset = (uint16_t)y * SCREEN_ROW_BYTES;
        memcpy(dest + offset, screen_buf + offset, SCREEN_ROW_BYTES);
    }
}

void screen_put_cell(uint8_t x, uint8_t y, uint8_t ch)
{
    uint16_t offset;

    if (x >= SCREEN_WIDTH || y >= SCREEN_HEIGHT) {
        return;
    }

    offset = (uint16_t)y * SCREEN_ROW_BYTES + x;
    screen_buf[offset] = ch;
}

void screen_playfield_clear(uint8_t max_row)
{
    uint8_t y;
    uint16_t base;

    for (y = 0; y < max_row; ++y) {
        base = (uint16_t)y * SCREEN_ROW_BYTES;
        /* Teletext blank mosaic (160), not ASCII space — space breaks graphics mode */
        memset(screen_buf + base + PLAYFIELD_COL_OFFSET, GFX_CHAR_EMPTY,
               PLAYFIELD_COLS);
        screen_buf[base] = gfx_colour;
    }
}

void gfx_show_shape(uint8_t shape_id, int8_t center_x, int8_t center_y,
                    uint8_t max_row)
{
    const GfxShapeDef *shape;
    uint8_t            w;
    uint8_t            h;
    uint8_t            row;
    uint8_t            col;
    int8_t             start_x;
    int8_t             start_y;
    int8_t             x;
    int8_t             y;
    uint8_t            ch;
    const uint8_t     *cells;
    uint16_t           idx;

    if (shape_id >= gfx_shape_count) {
        return;
    }

    shape    = &gfx_shapes[shape_id];
    w        = shape->width;
    h        = shape->height;
    cells    = shape->cells;

    start_x = center_x - (int8_t)(w >> 1) - 1;
    start_y = center_y - (int8_t)(w >> 1) - 1;
    if ((w & 1u) == 0u) {
        start_x++;
        start_y++;
    }

    for (row = 0; row < h; ++row) {
        y = start_y + (int8_t)row;
        if (y < 0 || y >= (int8_t)max_row) {
            continue;
        }

        idx = (uint16_t)row * w;
        for (col = 0; col < w; ++col) {
            ch = cells[idx + col];
            if (ch == 0) {
                continue;
            }

            x = start_x + (int8_t)col + (int8_t)PLAYFIELD_COL_OFFSET;
            if (x >= PLAYFIELD_COL_OFFSET && x < (int8_t)SCREEN_WIDTH) {
                screen_put_cell((uint8_t)x, (uint8_t)y, ch);
            }
        }
    }
}

void gfx_cycle_colour(void)
{
    gfx_colour++;
    if (gfx_colour > GFX_COLOUR_LAST) {
        gfx_colour = GFX_COLOUR_FIRST;
    }
}

uint8_t gfx_get_colour(void)
{
    return gfx_colour;
}
