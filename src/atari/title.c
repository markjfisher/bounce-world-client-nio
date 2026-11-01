#include <atari.h>
#include <conio.h>
#include <stdint.h>
#include <string.h>
#include "title.h"

#include "title_bitmap.h"

#define ART_ROWS 48
#define ART_STRIDE 40
static uint8_t spill[80];
static uint8_t *art_rows[ART_ROWS];
static uint8_t *title_dl;
static uint8_t *old_dl;
/* The game's 960-byte back buffer at $1000 is idle on the title screen. */
extern uint8_t display_buffer[];
static uint8_t last_tick, comet_x, comet_visible;
static uint8_t comet_saved[5];

/* ANTIC wraps inside a 4K block. Pad before any row that would cross one. */
static void make_art(void)
{
    uint8_t *src = title_bitmap;
    uint8_t *safe = spill;
    uint8_t row;
    if (((uint16_t)safe & 0x0fff) > 0x0fd8)
        safe = (uint8_t *)(((uint16_t)safe + 0x0fff) & 0xf000);
    for (row = 0; row < ART_ROWS; ++row) {
        if (((uint16_t)src & 0x0fff) > 0x0fd8) {
            memcpy(safe, src, ART_STRIDE);
            art_rows[row] = safe;
        } else {
            art_rows[row] = src;
        }
        src += ART_STRIDE;
    }
}
static void pixel(uint8_t x, uint8_t y, uint8_t colour)
{
    uint8_t *p = art_rows[y] + (x >> 2);
    uint8_t shift = (uint8_t)((3 - (x & 3)) * 2);
    *p = (uint8_t)((*p & ~(3 << shift)) | (colour << shift));
}
static uint8_t read_pixel(uint8_t x, uint8_t y)
{
    return (art_rows[y][x >> 2] >> ((3 - (x & 3)) * 2)) & 3;
}
static void comet_hide(void)
{
    if (!comet_visible) return;
    pixel(comet_x, 46, comet_saved[0]);
    pixel(comet_x + 1, 45, comet_saved[1]);
    pixel(comet_x + 1, 46, comet_saved[2]);
    pixel(comet_x + 2, 46, comet_saved[3]);
    pixel(comet_x + 1, 47, comet_saved[4]);
    comet_visible = 0;
}
static void comet_show(void)
{
    comet_saved[0] = read_pixel(comet_x, 46);
    comet_saved[1] = read_pixel(comet_x + 1, 45);
    comet_saved[2] = read_pixel(comet_x + 1, 46);
    comet_saved[3] = read_pixel(comet_x + 2, 46);
    comet_saved[4] = read_pixel(comet_x + 1, 47);
    pixel(comet_x, 46, 3);
    pixel(comet_x + 1, 45, 3);
    pixel(comet_x + 1, 46, 3);
    pixel(comet_x + 2, 46, 3);
    pixel(comet_x + 1, 47, 3);
    comet_visible = 1;
}
static void field_box(uint8_t row, const char *label)
{
    cputcxy(2, row, CH_ULCORNER);
    chlinexy(3, row, 34);
    cputcxy(37, row, CH_URCORNER);
    cputsxy(5, row, label);
    cputcxy(2, row + 1, CH_VLINE);
    cputcxy(37, row + 1, CH_VLINE);
    cputcxy(2, row + 2, CH_LLCORNER);
    chlinexy(3, row + 2, 34);
    cputcxy(37, row + 2, CH_LRCORNER);
}
void title_draw(void)
{
    clrscr();
    OS.color0 = 0x8a;
    OS.color1 = 0x0e;
    OS.color2 = 0x2a;
    OS.color4 = 0x00;
    make_art();
    comet_x = 0;
    comet_visible = 0;
    cputsxy(7, 8, "Choose a destination");
    field_box(10, " S  Server address ");
    field_box(14, " N  Pilot name ");
    cputsxy(7, 19, "S  Server     N  Pilot");
    revers(1);
    cputsxy(6, 21, "  Return  Launch world  ");
    revers(0);
    cputsxy(7, 23, "FujiNet NIO  /  Mark Fisher");
}
void title_server(const char *value)
{
    cputsxy(4, 11, "                                ");
    gotoxy(4, 11);
    if (!*value) cputs("(press S to enter)");
    while (*value && wherex() < 36) cputc(*value++);
}
void title_name(const char *value)
{
    cputsxy(4, 15, "                                ");
    cputsxy(4, 15, *value ? value : "(press N to enter)");
}
void title_enable(void)
{
    uint8_t i;
    uint16_t address;
    uint8_t *screen = *(uint8_t **)0x58;
    uint8_t *p;
    /* Reuse the page-aligned game back buffer. The display list cannot
     * cross ANTIC's 1K display-list boundary. */
    title_dl = display_buffer;
    p = title_dl;
    old_dl = (uint8_t *)OS.sdlst;
    *p++ = 0x70; *p++ = 0x70; *p++ = 0x70;
    for (i = 0; i < ART_ROWS; ++i) {
        address = (uint16_t)art_rows[i];
        *p++ = (i == ART_ROWS - 1) ? 0xce : 0x4e;
        *p++ = (uint8_t)address;
        *p++ = (uint8_t)(address >> 8);
    }
    for (i = 8; i < 24; ++i) {
        address = (uint16_t)(screen + (uint16_t)i * 40);
        *p++ = (i == 9 || i == 10 || i == 11 || i == 12 ||
                i == 13 || i == 14 || i == 15 || i == 16 ||
                i == 19 || i == 21 || i == 23) ? 0xc2 : 0x42;
        *p++ = (uint8_t)address;
        *p++ = (uint8_t)(address >> 8);
    }
    address = (uint16_t)title_dl;
    *p++ = 0x41; *p++ = (uint8_t)address; *p++ = (uint8_t)(address >> 8);
    OS.sdlst = title_dl;
    title_fx_start();
}
void title_disable(void)
{
    comet_hide();
    title_fx_stop();
    OS.sdlst = old_dl;
}
void title_animate(void)
{
    uint8_t tick = *(volatile uint8_t *)0x14;
    if ((uint8_t)(tick - last_tick) < 3) return;
    last_tick = tick;
    comet_hide();
    if (++comet_x >= 157) comet_x = 0;
    comet_show();
}
/* Keep long endpoints intact and scroll the visible part of the field. */
void title_edit(char *buffer, uint8_t capacity, uint8_t row)
{
    uint8_t n = 0, ch, offset;
    memset(buffer, 0, capacity + 1);
    for (;;) {
        offset = n > 30 ? n - 30 : 0;
        cputsxy(4, row, "                                ");
        cputsxy(4, row, buffer + offset);
        gotoxy((uint8_t)(4 + n - offset), row);
        cursor(1);
        while (!kbhit()) title_animate();
        ch = (uint8_t)cgetc();
        if (ch == CH_ENTER || ch == '\n') break;
        if (ch == CH_DEL || ch == CH_CURS_LEFT) {
            if (n) buffer[--n] = 0;
        } else if (ch >= 32 && ch < 127 && n < capacity - 1) {
            buffer[n++] = (char)ch;
            buffer[n] = 0;
        }
    }
    cursor(0);
}
