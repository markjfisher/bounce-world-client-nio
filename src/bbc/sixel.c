#include <stdint.h>
#include <string.h>

#include "sixel.h"

#ifndef SIXEL_STANDALONE
#include <conio.h>
#include "delay.h"
#else
#include <stdio.h>
static void gotoxy(uint8_t x, uint8_t y) { (void)x; (void)y; }
static void cputc(char c) { (void)c; }
static void clrscr(void) {}
static void cputs(const char *s) { (void)s; }
static void cputsxy(uint8_t x, uint8_t y, const char *s) { (void)x; (void)y; (void)s; }
#endif

static uint16_t sixel_bit_index(int16_t x, int16_t y)
{
    return (uint16_t)(y * SIXEL_COLS + x);
}

static uint8_t shape_bit_at(const SixelShapeDef *shape, uint8_t x, uint8_t y)
{
    uint16_t bit;
    uint8_t  byte;

    if (x >= shape->width || y >= shape->height) {
        return 0;
    }

    bit  = (uint16_t)(y * shape->width + x);
    byte = shape->bits[(uint8_t)(bit >> 3)];
    return (uint8_t)((byte >> (7 - (bit & 7))) & 1);
}

void sixel_grid_clear(SixelGrid *grid)
{
    memset(grid->bits, 0, sizeof(grid->bits));
}

void sixel_grid_set(SixelGrid *grid, int16_t x, int16_t y, uint8_t on)
{
    uint16_t bit;
    uint8_t  mask;

    if (x < 0 || y < 0 || x >= SIXEL_COLS || y >= SIXEL_ROWS) {
        return;
    }

    bit  = sixel_bit_index(x, y);
    mask = (uint8_t)(0x80 >> (bit & 7));

    if (on) {
        grid->bits[(uint8_t)(bit >> 3)] |= mask;
    } else {
        grid->bits[(uint8_t)(bit >> 3)] &= (uint8_t)~mask;
    }
}

uint8_t sixel_grid_get(const SixelGrid *grid, int16_t x, int16_t y)
{
    uint16_t bit;
    uint8_t  mask;

    if (x < 0 || y < 0 || x >= SIXEL_COLS || y >= SIXEL_ROWS) {
        return 0;
    }

    bit  = sixel_bit_index(x, y);
    mask = (uint8_t)(0x80 >> (bit & 7));
    return (uint8_t)((grid->bits[(uint8_t)(bit >> 3)] & mask) ? 1 : 0);
}

void sixel_blit_shape(SixelGrid *grid, const SixelShapeDef *shape,
                      int16_t center_x, int16_t center_y)
{
    uint8_t x;
    uint8_t y;
    int16_t wx;
    int16_t wy;

    for (y = 0; y < shape->height; ++y) {
        for (x = 0; x < shape->width; ++x) {
            if (!shape_bit_at(shape, x, y)) {
                continue;
            }

            wx = center_x + (int16_t)x - (int16_t)shape->center_x;
            wy = center_y + (int16_t)y - (int16_t)shape->center_y;
            sixel_grid_set(grid, wx, wy, 1);
        }
    }
}

uint8_t sixel_cell_code(const SixelGrid *grid, uint8_t col, uint8_t row)
{
    int16_t sx;
    int16_t sy;
    uint8_t code;

    sx = (int16_t)col * 2;
    sy = (int16_t)row * 3;

    code = SIXEL_CHAR_BASE;
    if (sixel_grid_get(grid, sx,     sy))     code |= 1;
    if (sixel_grid_get(grid, sx + 1, sy))     code |= 2;
    if (sixel_grid_get(grid, sx,     sy + 1)) code |= 4;
    if (sixel_grid_get(grid, sx + 1, sy + 1)) code |= 8;
    if (sixel_grid_get(grid, sx,     sy + 2)) code |= 16;
    if (sixel_grid_get(grid, sx + 1, sy + 2)) code |= 32;

    return code;
}

void sixel_render_grid(const SixelGrid *grid, uint8_t max_row)
{
    uint8_t col;
    uint8_t row;
    uint8_t code;

    for (row = 0; row < max_row; ++row) {
        gotoxy(0, row);
        for (col = 0; col < SCREEN_WIDTH; ++col) {
            code = sixel_cell_code(grid, col, row);
            if (code == SIXEL_CHAR_BASE) {
                cputc(' ');
            } else {
                cputc((char)code);
            }
        }
    }
}

/*
 * Local sixel shape table (server shape IDs index into this).
 * Replace / extend with real bounce-world shape art.
 */

/* 6x4 diamond-ish test shape (matches the example in design notes):
 *   .. x.
 *   .x .x
 *   x. .x
 *   .x x.
 */
static const uint8_t shape0_bits[] = {
    0b00100000, 0b01010000,
    0b10001000, 0b01100000,
};

// width, height, centre_x, centre_y, bits data
static const SixelShapeDef sixel_shapes[] = {
    { 6, 4, 3, 2, shape0_bits },
};

#define SIXEL_SHAPE_COUNT (sizeof(sixel_shapes) / sizeof(sixel_shapes[0]))

void sixel_render_objects(const uint8_t *objects, uint8_t count,
                          uint8_t max_row)
{
    static SixelGrid grid;
    uint8_t i;
    uint8_t shape_id;
    int16_t cx;
    int16_t cy;

    sixel_grid_clear(&grid);

    for (i = 0; i < count; ++i) {
        shape_id = objects[i * 3];
        cx       = (int16_t)objects[i * 3 + 1];
        cy       = (int16_t)objects[i * 3 + 2];

        if (shape_id < SIXEL_SHAPE_COUNT) {
            sixel_blit_shape(&grid, &sixel_shapes[shape_id], cx, cy);
        }
    }

    sixel_render_grid(&grid, max_row);
}

#ifndef SIXEL_STANDALONE
void sixel_run_demo(void)
{
    static SixelGrid grid;
    int16_t cx;
    int16_t cy;
    uint8_t step;

    clrscr();
    cputs("Sixel sub-char demo - press any key");

    /* Start centred on the sixel grid, then slide 6 sixels left. */
    cx = 40;
    cy = 36;

    for (step = 0; step < 7; ++step) {
        sixel_grid_clear(&grid);
        sixel_blit_shape(&grid, &sixel_shapes[0], cx, cy);
        sixel_render_grid(&grid, SCREEN_HEIGHT);
        cx--;
        pause(40);
    }

    cputsxy(0, SCREEN_HEIGHT - 1, "Done.                ");
}
#endif /* !SIXEL_STANDALONE */

#ifdef SIXEL_STANDALONE
#include <unistd.h>

static char sixel_to_ascii(uint8_t code)
{
    static const char glyphs[] = " .':;oO@";
    uint8_t n;

    if (code <= SIXEL_CHAR_BASE) {
        return ' ';
    }
    n = (uint8_t)(code - SIXEL_CHAR_BASE);
    return glyphs[(n * 7) / 63];
}

static void term_render_grid(const SixelGrid *grid, uint8_t max_row)
{
    uint8_t col;
    uint8_t row;
    uint8_t code;

    printf("\033[2J\033[H");
    for (row = 0; row < max_row; ++row) {
        for (col = 0; col < SCREEN_WIDTH; ++col) {
            code = sixel_cell_code(grid, col, row);
            putchar(sixel_to_ascii(code));
        }
        putchar('\n');
    }
    fflush(stdout);
}

int main(void)
{
    SixelGrid grid;
    int16_t cx = 40;
    int16_t cy = 36;
    uint8_t step;

    for (step = 0; step < 7; ++step) {
        sixel_grid_clear(&grid);
        sixel_blit_shape(&grid, &sixel_shapes[0], cx, cy);
        term_render_grid(&grid, 12);
        printf("step %u centre=(%d,%d)\n", step, cx, cy);
        cx--;
        usleep(200000);
    }

    return 0;
}
#endif /* SIXEL_STANDALONE */
