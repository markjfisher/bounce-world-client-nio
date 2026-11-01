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

/* Slide each object down inside its own cell, so settled neighbours survive. */
static void draw_arrival(uint8_t index, uint8_t shift)
{
    ShapeRecord *shape = &shapes[index];
    uint8_t row, col, width = shape->shape_width;
    uint8_t x = (uint8_t)((index % PREVIEW_COLS) * PREVIEW_COL_SP + 1);
    uint8_t y = (uint8_t)((index / PREVIEW_COLS) * PREVIEW_ROW_SP + PREVIEW_ORIGIN_Y);
    uint8_t *screen = *(uint8_t **)0x58;
    for (row = 0; row < width; ++row) {
        uint8_t *dest = screen + (uint16_t)(y + row) * 40 + x;
        for (col = 0; col < width; ++col) {
            dest[col] = row < shift ? 0 : screen_code(shape->shape_data[(row - shift) * width + col]);
        }
    }
}

void show_shapes_preview(void)
{
    uint8_t i, shift;
    shapes_load_embedded();
    clrscr();
    OS.color1 = 0x0e;
    OS.color2 = 0x82;
    OS.color4 = 0x00;
    cputsxy(0, 1, " .      +      .     +        .      +");
    revers(1);
    cputsxy(6, 0, "  BOUNCY WORLD / OBJECTS  ");
    revers(0);
    cputsxy(7, 21, "OBJECTS ENTERING ORBIT");
    revers(1);
    cputsxy(7, 23, " PRESS ANY KEY TO LAUNCH ");
    revers(0);
    last_ping = ticks();

    for (i = 0; i < shape_count && !kbhit(); ++i) {
        shift = shapes[i].shape_width;
        do {
            --shift;
            draw_arrival(i, shift);
            preview_frame();
            preview_frame();
        } while (shift && !kbhit());
    }
    /* Keep a gentle wave moving through the settled objects. */
    i = 0;
    while (!kbhit()) {
        if (shapes[i].shape_width > 2) {
            uint8_t frame;
            draw_arrival(i, 1);
            for (frame = 0; frame < 4 && !kbhit(); ++frame) preview_frame();
            draw_arrival(i, 0);
            for (frame = 0; frame < 4 && !kbhit(); ++frame) preview_frame();
        }
        if (++i == shape_count) i = 0;
    }
    cgetc();
    clrscr();
}
