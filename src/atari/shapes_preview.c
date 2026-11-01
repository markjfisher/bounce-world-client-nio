#include <conio.h>
#include <atari.h>
#include <stdint.h>

#include "delay.h"
#include "shapes.h"
#include "world.h"

#define PREVIEW_COLS 7
#define PREVIEW_COL_SP 5
#define PREVIEW_ROW_SP 6
#define PREVIEW_ORIGIN_Y 3
/* 1,000 video frames: 20 seconds PAL, about 17 seconds NTSC. */
#define KEEPALIVE_FRAMES 1000

static uint16_t ticks(void)
{
    volatile uint8_t *clock = (volatile uint8_t *)0x13;
    uint8_t high, low;
    do { high = clock[0]; low = clock[1]; } while (high != clock[0]);
    return ((uint16_t)high << 8) | low;
}

static uint16_t last_ping;
static void preview_frame(void)
{
    wait_vsync();
    if ((uint16_t)(ticks() - last_ping) >= KEEPALIVE_FRAMES) {
        fetch_client_state();
        last_ping = ticks();
    }
}

/* Convert ATASCII to the ROM character-set screen code. */
static uint8_t screen_code(uint8_t ch)
{
    uint8_t inverse = ch & 0x80;
    ch &= 0x7f;
    if (ch < 32) ch += 64;
    else if (ch < 96) ch -= 32;
    return ch | inverse;
}

/* Save-under lets an arriving object cross the settled ones without erasing them. */
static uint8_t saved[25];
static void sprite(uint8_t index, uint8_t x, uint8_t restore)
{
    ShapeRecord *shape = &shapes[index];
    uint8_t row, col, width = shape->shape_width, k = 0;
    uint8_t y = (uint8_t)((index / PREVIEW_COLS) * PREVIEW_ROW_SP + PREVIEW_ORIGIN_Y);
    uint8_t *screen = *(uint8_t **)0x58;
    for (row = 0; row < width; ++row) {
        uint8_t *line = screen + (uint16_t)(y + row) * 40;
        for (col = 0; col < width; ++col, ++k) {
            uint8_t pos = x + col;
            uint8_t ch;
            if (pos >= 40) continue;
            if (restore) { line[pos] = saved[k]; continue; }
            saved[k] = line[pos];
            ch = screen_code(shape->shape_data[k]);
            if (ch) line[pos] = ch;
        }
    }
}
static void clear_objects(void)
{
    uint8_t row;
    uint8_t *screen = *(uint8_t **)0x58;
    for (row = 3; row < 20; ++row) {
        uint8_t col;
        uint8_t *line = screen + (uint16_t)row * 40;
        for (col = 0; col < 40; ++col) line[col] = 0;
    }
}

void show_shapes_preview(void)
{
    uint8_t i, x, target, frame;
    shapes_load_embedded();
    clrscr();
    OS.color1 = 0x0e;
    OS.color2 = 0x82;
    OS.color4 = 0x00;
    cputsxy(0, 1, " .      +      .     +        .      +");
    revers(1);
    cputsxy(6, 0, "  Bouncy World / Objects  ");
    revers(0);
    cputsxy(8, 21, "Incoming objects");
    revers(1);
    cputsxy(8, 23, " Any key: launch world ");
    revers(0);
    last_ping = ticks();

    while (!kbhit()) {
        clear_objects();
        for (i = 0; i < shape_count && !kbhit(); ++i) {
            target = (uint8_t)((i % PREVIEW_COLS) * PREVIEW_COL_SP + 1);
            x = 40;
            for (;;) {
                sprite(i, x, 0);
                preview_frame();
                if (kbhit() || x == target) break;
                sprite(i, x, 1);
                x = (x - target > 2) ? x - 2 : target;
            }
        }
        for (frame = 0; frame < 100 && !kbhit(); ++frame) preview_frame();
    }
    cgetc();
    clrscr();
}
