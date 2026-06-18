#include <conio.h>
#include <stdint.h>
#include <string.h>

#include "data.h"
#include "delay.h"
#include "gfx_shapes.h"
#include "screen.h"
#include "world.h"

/* MODE 7 teletext control codes in screen RAM (ISS / edit.tf). */
#define TXT_DH_ON         (0x0D)
#define TXT_RED           (0x01)
#define TXT_GREEN         (0x02)
#define TXT_YELLOW        (0x03)
#define TXT_BLUE          (0x04)
#define TXT_MAGENTA       (0x05)
#define TXT_CYAN          (0x06)
#define TXT_WHITE         (0x07)
#define TXT_SPACE         (0x20)

#define PREVIEW_COLS          (7)
#define PREVIEW_COL_SP        (5)
#define PREVIEW_ROW_SP        (6)
#define PREVIEW_ORIGIN_X      (3)
#define PREVIEW_ORIGIN_Y      (8)   /* cy for row 0; 5x5 needs cy>=3 */
#define PREVIEW_SHAPE_MAX_ROW (22)

#define ROW_TITLE_DH          (0)
#define ROW_BLANK1            (2)
#define ROW_SUBTITLE          (3)
#define ROW_BLANK2            (4)
#define ROW_PROMPT            (22)

static uint8_t *row_ptr(uint8_t y)
{
    return &screen_buf[(uint16_t)y * SCREEN_WIDTH];
}

static void fill_row(uint8_t y, uint8_t ch)
{
    memset(row_ptr(y), ch, SCREEN_WIDTH);
}

static void write_centred(uint8_t y, uint8_t colour, const char *text)
{
    uint8_t  len   = (uint8_t)strlen(text);
    uint8_t  start = (uint8_t)((SCREEN_WIDTH - len) / 2);
    uint8_t *row   = row_ptr(y);
    uint8_t  x;

    fill_row(y, TXT_SPACE);
    row[0] = colour;
    for (x = 0; x < len; ++x) {
        row[start + x] = (uint8_t)text[x];
    }
}

static void write_double_height_centred(uint8_t y, uint8_t colour, const char *text)
{
    uint8_t  len   = (uint8_t)strlen(text);
    uint8_t  start = (uint8_t)(2 + (SCREEN_WIDTH - 2 - len) / 2);
    uint8_t *top   = row_ptr(y);
    uint8_t  x;

    fill_row(y, TXT_SPACE);
    fill_row((uint8_t)(y + 1), TXT_SPACE);

    top[0] = TXT_DH_ON;
    top[1] = colour;
    for (x = 0; x < len; ++x) {
        top[start + x] = (uint8_t)text[x];
    }
}

static void draw_preview_text(void)
{
    write_double_height_centred(ROW_TITLE_DH, TXT_CYAN, "Bouncy World");
    fill_row(ROW_BLANK1, TXT_SPACE);
    write_centred(ROW_SUBTITLE, TXT_YELLOW, "Bouncy World shapes");
    fill_row(ROW_BLANK2, TXT_SPACE);
    write_centred(ROW_PROMPT, TXT_WHITE, "Press a key to continue");
}

static void draw_preview_shapes(void)
{
    uint8_t i;
    uint8_t cx;
    uint8_t cy;

    for (i = 0; i < gfx_shape_count; ++i) {
        cx = (uint8_t)((i % PREVIEW_COLS) * PREVIEW_COL_SP + PREVIEW_ORIGIN_X);
        cy = (uint8_t)((i / PREVIEW_COLS) * PREVIEW_ROW_SP + PREVIEW_ORIGIN_Y);
        gfx_show_shape(i, (int8_t)cx, (int8_t)cy, PREVIEW_SHAPE_MAX_ROW);
    }
}

void show_shapes_preview(void)
{
    screen_init();
    shape_count = gfx_shape_count;

    while (kbhit() == 0) {
        gfx_cycle_colour();
        screen_playfield_clear(SCREEN_HEIGHT);
        draw_preview_text();
        draw_preview_shapes();
        screen_blit_rows(SCREEN_HEIGHT);
        fetch_client_state();
        pause(10);
    }

    cgetc();
    clrscr();
    gfx_colour = GFX_COLOUR_DEFAULT;
    screen_playfield_clear(SCREEN_HEIGHT);
}
