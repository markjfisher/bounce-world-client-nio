#include <bios.h>
#include <conio.h>
#include <dos.h>
#include <stdio.h>
#include <stdlib.h>

#include "conio.h"

static uint8_t cursor_x = 0;
static uint8_t cursor_y = 0;
static uint8_t reverse_on = 0;

int putch(int c);

static void bios_set_cursor(uint8_t x, uint8_t y)
{
    union REGS regs;

    regs.h.ah = 0x02;
    regs.h.bh = 0x00;
    regs.h.dh = y;
    regs.h.dl = x;
    int86(0x10, &regs, &regs);
}

static void bios_clear_screen(void)
{
    union REGS regs;

    regs.h.ah = 0x06;
    regs.h.al = 0x00;
    regs.h.bh = 0x07;
    regs.h.ch = 0x00;
    regs.h.cl = 0x00;
    regs.h.dh = 24;
    regs.h.dl = 79;
    int86(0x10, &regs, &regs);
    bios_set_cursor(0, 0);
}

static void bios_set_cursor_visible(uint8_t on)
{
    union REGS regs;

    regs.h.ah = 0x01;
    if (on) {
        regs.h.ch = 0x06;
        regs.h.cl = 0x07;
    } else {
        regs.h.ch = 0x20;
        regs.h.cl = 0x00;
    }
    int86(0x10, &regs, &regs);
}

static void update_cursor_after_char(char c)
{
    if (c == '\r') {
        cursor_x = 0;
    } else if (c == '\n') {
        cursor_x = 0;
        cursor_y++;
    } else {
        cursor_x++;
    }
}

void clrscr(void)
{
    bios_clear_screen();
    cursor_x = 0;
    cursor_y = 0;
}

void gotoxy(uint8_t x, uint8_t y)
{
    bios_set_cursor(x, y);
    cursor_x = x;
    cursor_y = y;
}

void gotox(uint8_t x)
{
    gotoxy(x, cursor_y);
}

uint8_t wherex(void)
{
    return cursor_x;
}

uint8_t wherey(void)
{
    return cursor_y;
}

void cputc(char c)
{
    (void)reverse_on;
    putch((unsigned char)c);
    update_cursor_after_char(c);
}

void cputs(const char *s)
{
    while (*s != '\0') {
        cputc(*s++);
    }
}

void cputcxy(uint8_t x, uint8_t y, char c)
{
    gotoxy(x, y);
    cputc(c);
}

void cputsxy(uint8_t x, uint8_t y, const char *s)
{
    gotoxy(x, y);
    cputs(s);
}

void revers(uint8_t on)
{
    reverse_on = on;
}

void cursor(uint8_t on)
{
    bios_set_cursor_visible(on);
}

void chlinexy(uint8_t x, uint8_t y, uint8_t len)
{
    uint8_t i;

    gotoxy(x, y);
    for (i = 0; i < len; ++i) {
        cputc(CH_HLINE);
    }
}

uint8_t kbhit(void)
{
    return _bios_keybrd(_KEYBRD_READY) ? 1U : 0U;
}

char cgetc(void)
{
    unsigned key;
    unsigned char ascii;

    key = _bios_keybrd(_KEYBRD_READ);
    ascii = (unsigned char)(key & 0xffU);
    if (ascii == 0) {
        return 0;
    }
    if (ascii == '\r') {
        return (char)CH_ENTER;
    }
    return (char)ascii;
}
