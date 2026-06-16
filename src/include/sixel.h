#ifndef BWC_SIXEL_H
#define BWC_SIXEL_H

#include <stdint.h>
#include "screen.h"

/*
 * Teletext (MODE 7) sixel rendering.
 *
 * Each character cell is a 2-wide x 3-tall grid of "sixels".  Codes 160-255
 * encode which of the six pixels are set (160 = blank, 255 = all filled).
 *
 * The server reports object centres on an 80x72 sixel grid (40x24 characters).
 * Shapes are defined locally in sixels; we composite them into a frame buffer
 * and convert affected character cells to teletext graphics codes.
 */

#define SIXEL_COLS (SCREEN_WIDTH * 2)   /* 80 */
#define SIXEL_ROWS (SCREEN_HEIGHT * 3)  /* 72 */
#define SIXEL_BYTES ((SIXEL_COLS * SIXEL_ROWS + 7) / 8)

#define SIXEL_CHAR_BASE 160

typedef struct {
    uint8_t  width;   /* sixel columns */
    uint8_t  height;  /* sixel rows */
    int8_t   center_x;
    int8_t   center_y;
    const uint8_t *bits; /* row-major, MSB of first byte = (0,0) */
} SixelShapeDef;

typedef struct {
    uint8_t bits[SIXEL_BYTES];
} SixelGrid;

void sixel_grid_clear(SixelGrid *grid);

void sixel_grid_set(SixelGrid *grid, int16_t x, int16_t y, uint8_t on);

uint8_t sixel_grid_get(const SixelGrid *grid, int16_t x, int16_t y);

void sixel_blit_shape(SixelGrid *grid, const SixelShapeDef *shape,
                      int16_t center_x, int16_t center_y);

uint8_t sixel_cell_code(const SixelGrid *grid, uint8_t col, uint8_t row);

void sixel_render_grid(const SixelGrid *grid, uint8_t max_row);

void sixel_render_objects(const uint8_t *objects, uint8_t count,
                          uint8_t max_row);

/* Interactive demo: animates a sample shape sliding in sixel steps. */
void sixel_run_demo(void);

#endif /* BWC_SIXEL_H */
