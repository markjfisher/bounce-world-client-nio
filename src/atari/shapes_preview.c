#include <conio.h>
#include <atari.h>
#include <stdint.h>
#include <string.h>

#include "data.h"
#include "delay.h"
#include "shapes.h"
#include "world.h"

#define PREVIEW_COLS  (7)
#define PREVIEW_COL_SP (6)
#define PREVIEW_ROW_SP (6)
#define PREVIEW_ORIGIN_Y (3)

static void draw_preview_shapes(void)
{
    uint8_t i;
    uint8_t x;
    uint8_t y;

    for (i = 0; i < shape_count; ++i) {
        x = (uint8_t)((i % PREVIEW_COLS) * PREVIEW_COL_SP);
        y = (uint8_t)((i / PREVIEW_COLS) * PREVIEW_ROW_SP + PREVIEW_ORIGIN_Y);
        display_shape_data(i, x, y);
    }
}

void show_shapes_preview(void)
{
    shapes_load_embedded();

    clrscr();
    OS.color1 = 0x0e;
    OS.color2 = 0x92;
    OS.color4 = 0x00;
    cputsxy(0, 1, " .      *      .     *        .      *");
    revers(1);
    cputsxy(8, 0, " BOUNCY WORLD / OBJECTS ");
    revers(0);
    draw_preview_shapes();
    revers(1);
    cputsxy(8, 21, "Press a key to continue");
    revers(0);

    while (kbhit() == 0) {
        static const uint8_t space_colours[] = {0x92, 0x82, 0x42, 0xc2};
        OS.color2 = space_colours[(*(volatile uint8_t *)0x14 >> 6) & 3];
        fetch_client_state();
        pause(20);
    }

    cgetc();
    clrscr();
}
