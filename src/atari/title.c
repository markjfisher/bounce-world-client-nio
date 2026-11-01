#include <atari.h>
#include <conio.h>
#include <stdint.h>
#include <string.h>
#include "title.h"

/* Five by five pixel letters. Each pixel is an inverse text cell. */
static const uint8_t letters[][5] = {
    {30,17,30,17,30}, /* B */ {14,17,17,17,14}, /* O */
    {17,17,17,17,14}, /* U */ {17,25,21,19,17}, /* N */
    {15,16,16,16,15}, /* C */ {17,10,4,4,4},   /* Y */
    {17,17,21,27,17}, /* W */ {30,17,30,18,17}, /* R */
    {16,16,16,16,31}, /* L */ {30,17,17,17,30}  /* D */
};
static const uint8_t top[] = {0,1,2,3,4,5};
static const uint8_t bottom[] = {6,1,7,8,9};
static const uint8_t band_rows[] = {4,9,11,14,17,20,23};
static uint8_t old_ops[24];
static uint8_t *dl;
static uint8_t last_tick, noise = 117;

static void word(const uint8_t *glyphs, uint8_t count, uint8_t y)
{
    uint8_t n, row, col, bits;
    uint8_t x = (uint8_t)((40 - count * 6 + 1) / 2);
    revers(1);
    for (n = 0; n < count; ++n)
        for (row = 0; row < 5; ++row) {
            bits = letters[glyphs[n]][row];
            for (col = 0; col < 5; ++col)
                if (bits & (16 >> col)) cputcxy(x + n * 6 + col, y + row, ' ');
        }
    revers(0);
}
void title_draw(void)
{
    clrscr();
    OS.color1 = 0x0e;
    OS.color2 = 0x02;
    OS.color4 = 0x00;
    word(top, 6, 0);
    word(bottom, 5, 5);
    cputsxy(2, 11, "  CONNECT TO THE BOUNCY UNIVERSE");
    cputsxy(2, 12, "SERVER  [S] EDIT");
    cputsxy(2, 15, "PILOT   [N] EDIT");
    cputsxy(2, 19, "  *    .   +     .   *     +");
    revers(1);
    cputsxy(3, 21, "  RETURN / SPACE : LAUNCH WORLD   ");
    revers(0);
    cputsxy(5, 23, "FUJINET NIO  /  MARK FISHER");
}
void title_server(const char *value)
{
    cputsxy(3, 13, "                                  ");
    gotoxy(3, 13);
    while (*value && wherex() < 36) cputc(*value++);
}
void title_name(const char *value)
{
    cputsxy(3, 16, "                                  ");
    cputsxy(3, 16, value);
}
void title_enable(void)
{
    uint8_t i, off;
    dl = (uint8_t *)OS.sdlst;
    for (i = 0; i < 24; ++i) {
        off = i ? 5 + i : 3;
        old_ops[i] = dl[off];
        dl[off] &= 0x7f;
    }
    for (i = 0; i < 7; ++i) dl[5 + band_rows[i]] |= 0x80;
    title_fx_start();
}
void title_disable(void)
{
    uint8_t i;
    title_fx_stop();
    for (i = 0; i < 24; ++i) dl[i ? 5 + i : 3] = old_ops[i];
}
void title_animate(void)
{
    uint8_t tick = *(volatile uint8_t *)0x14, i, r, c;
    uint8_t *screen = *(uint8_t **)0x58;
    if (tick == last_tick) return;
    last_tick = tick;
    for (i = 0; i < 8; ++i) {
        noise ^= noise << 3; noise ^= noise >> 5; noise ^= noise << 1;
        r = noise % 10;
        noise ^= noise << 3; noise ^= noise >> 5; noise ^= noise << 1;
        c = noise % 40;
        { uint8_t *cell = screen + (uint16_t)r * 40 + c;
          if (*cell == 0) *cell = 10;
          else if (*cell == 10) *cell = 0; }
    }
}
/* Keep long endpoints intact and scroll the visible part of the field. */
void title_edit(char *buffer, uint8_t capacity, uint8_t row)
{
    uint8_t n = 0, ch, offset;
    memset(buffer, 0, capacity + 1);
    for (;;) {
        offset = n > 30 ? n - 30 : 0;
        cputsxy(3, row, "                                  ");
        cputsxy(3, row, buffer + offset);
        gotoxy((uint8_t)(3 + n - offset), row);
        cursor(1);
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
