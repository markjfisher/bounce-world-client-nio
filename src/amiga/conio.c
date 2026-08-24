#include <exec/types.h>
#include <exec/libraries.h>
#include <intuition/intuition.h>
#include <intuition/screens.h>
#include <devices/inputevent.h>
#include <graphics/gfx.h>
#include <graphics/displayinfo.h>
#include <graphics/gfxbase.h>
#include <graphics/text.h>
#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/intuition.h>

#include <stdint.h>
#include <stdlib.h>

#include "screen.h"
#include "conio.h"

/* Classic NDK globals the proto/inline headers reference */
struct GfxBase *GfxBase = NULL;
struct IntuitionBase *IntuitionBase = NULL;

static struct Screen *scr;
static struct Window *win;
static struct TextFont *font;
static struct ScreenBuffer *sbuf[2];
static struct RastPort brp[2];
static uint16_t screen_h_px;
static uint8_t cur_buf;
static uint8_t cur_x, cur_y;
static uint8_t cur_revers;
static uint8_t cur_cursor_visible;
static uint8_t screen_ready;

void *amiga_conio_draw_rp(void)
{
    if (!screen_ready) {
        return NULL;
    }
    return &brp[cur_buf];
}

static void set_text_pens(struct RastPort *rp)
{
    if (cur_revers) {
        SetAPen(rp, 0);
        SetBPen(rp, 1);
    } else {
        SetAPen(rp, 1);
        SetBPen(rp, 0);
    }
    SetDrMd(rp, JAM2);
}

void amiga_conio_clear(void)
{
    struct RastPort *rp = amiga_conio_draw_rp();

    if (!rp) {
        return;
    }
    SetAPen(rp, 0);
    RectFill(rp, 0, 0, SCREEN_PIXEL_WIDTH - 1, screen_h_px - 1);
    cur_x = 0;
    cur_y = 0;
}

static void draw_char(char c)
{
    struct RastPort *rp = amiga_conio_draw_rp();

    if (!rp) {
        return;
    }
    if (c == '\n') {
        cur_x = 0;
        cur_y++;
    } else {
        set_text_pens(rp);
        Move(rp, (LONG)(cur_x * 8), (LONG)(cur_y * 8 + 7));
        Text(rp, (CONST_STRPTR)&c, 1);
        cur_x++;
    }
    if (cur_x >= SCREEN_WIDTH) {
        cur_x = 0;
        cur_y++;
    }
    if (cur_y >= SCREEN_HEIGHT) {
        cur_y = SCREEN_HEIGHT - 1;
        cur_x = 0;
    }
}

void amiga_conio_present(void)
{
    if (screen_ready) {
        ChangeScreenBuffer(scr, sbuf[cur_buf]);
    }
}

uint16_t amiga_conio_height(void)
{
    return screen_h_px;
}

void amiga_conio_swap(void)
{
    cur_buf ^= 1U;
}

void clrscr(void)
{
    amiga_conio_clear();
}

void gotoxy(uint8_t x, uint8_t y)
{
    cur_x = x;
    cur_y = y;
}

void gotox(uint8_t x)
{
    cur_x = x;
}

uint8_t wherex(void)
{
    return cur_x;
}

uint8_t wherey(void)
{
    return cur_y;
}

void cputc(char c)
{
    draw_char(c);
}

void cputs(const char *s)
{
    while (*s) {
        draw_char(*s++);
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
    cur_revers = on ? 1U : 0U;
}

void cursor(uint8_t on)
{
    cur_cursor_visible = on ? 1U : 0U;
}

void chlinexy(uint8_t x, uint8_t y, uint8_t len)
{
    uint8_t i;

    gotoxy(x, y);
    for (i = 0; i < len; ++i) {
        cputc(CH_HLINE);
    }
}

static char rawkey_to_ascii(uint16_t code, uint16_t qualifier)
{
    static const char *row_q = "qwertyuiop";
    static const char *row_a = "asdfghjkl";
    static const char *row_z = "zxcvbnm";
    char c = '\0';

    /* Ignore key-release events (IECODE_UP_PREFIX set) */
    if (code & IECODE_UP_PREFIX) {
        return '\0';
    }

    if (code <= 0x3F) {
        if (code >= 0x01 && code <= 0x0A) {
            c = "1234567890"[code - 0x01];
        } else if (code >= 0x10 && code <= 0x19) {
            c = row_q[code - 0x10];
        } else if (code >= 0x20 && code <= 0x28) {
            c = row_a[code - 0x20];
        } else if (code >= 0x30 && code <= 0x36) {
            c = row_z[code - 0x30];
        } else {
            switch (code) {
                case 0x00: c = '`'; break;
                case 0x0B: c = '-'; break;
                case 0x0C: c = '='; break;
                case 0x0D: c = '\\'; break;
                case 0x3B: c = ','; break;
                case 0x3C: c = '.'; break;
                case 0x3D: c = '/'; break;
                case 0x3E: c = '+'; break;
                case 0x3A: c = '-'; break;
                default: break;
            }
        }

        if (c != '\0' && (qualifier & (IEQUALIFIER_LSHIFT | IEQUALIFIER_RSHIFT))) {
            if (c >= 'a' && c <= 'z') {
                c = (char)(c - 'a' + 'A');
            }
        }
    } else {
        switch (code) {
            case 0x40: c = ' '; break;
            case 0x41: c = CH_DEL; break;
            case 0x42: c = '\t'; break;
            case 0x43: c = CH_ENTER; break;
            case 0x44: c = CH_ENTER; break;
            case 0x45: c = CH_STOP; break;
            default: break;
        }
    }

    return c;
}

uint8_t kbhit(void)
{
    if (!win) {
        return 0;
    }
    return GetMsg(win->UserPort) != NULL ? 1U : 0U;
}

char cgetc(void)
{
    struct IntuiMessage *msg;
    char result = '\0';

    if (!win) {
        return '\0';
    }

    for (;;) {
        msg = (struct IntuiMessage *)GetMsg(win->UserPort);
        if (msg) {
            uint16_t class_ = msg->Class;
            uint16_t code = msg->Code;
            uint16_t qualifier = msg->Qualifier;
            ReplyMsg((struct Message *)msg);

            if (class_ == IDCMP_CLOSEWINDOW) {
                return 'q';
            }
            if (class_ == IDCMP_RAWKEY) {
                result = rawkey_to_ascii(code, qualifier);
                if (result != '\0') {
                    return result;
                }
            }
            continue;
        }
        Wait(1UL << win->UserPort->mp_SigBit);
    }
}

static void close_all(void)
{
    if (win) {
        CloseWindow(win);
        win = NULL;
    }
    if (sbuf[1]) {
        FreeScreenBuffer(scr, sbuf[1]);
        sbuf[1] = NULL;
    }
    if (sbuf[0]) {
        FreeScreenBuffer(scr, sbuf[0]);
        sbuf[0] = NULL;
    }
    if (font) {
        CloseFont(font);
        font = NULL;
    }
    if (scr) {
        CloseScreen(scr);
        scr = NULL;
    }
    if (IntuitionBase) {
        CloseLibrary((struct Library *)IntuitionBase);
        IntuitionBase = NULL;
    }
    if (GfxBase) {
        CloseLibrary((struct Library *)GfxBase);
        GfxBase = NULL;
    }
    screen_ready = 0;
}

__attribute__((constructor)) static void amiga_screen_open(void)
{
    ULONG mode_id;
    LONG pal;

    IntuitionBase = (struct IntuitionBase *)OpenLibrary((CONST_STRPTR)"intuition.library", 37);
    GfxBase = (struct GfxBase *)OpenLibrary((CONST_STRPTR)"graphics.library", 37);
    if (!IntuitionBase || !GfxBase) {
        close_all();
        return;
    }

    pal = (GfxBase->DisplayFlags & PAL) ? 1 : 0;
    screen_h_px = pal ? 256 : 200;
    mode_id = (pal ? PAL_MONITOR_ID : NTSC_MONITOR_ID) | LORES_KEY;

    scr = OpenScreenTags(NULL,
                         SA_DisplayID, mode_id,
                         SA_Width, SCREEN_PIXEL_WIDTH,
                         SA_Height, screen_h_px,
                         SA_Depth, 3,
                         SA_Type, CUSTOMSCREEN,
                         SA_Quiet, TRUE,
                         SA_Behind, FALSE,
                         TAG_DONE);
    if (!scr) {
        close_all();
        return;
    }

    font = OpenFont((struct TextAttr *)&((struct TextAttr){ (STRPTR)"topaz.font", 8, FS_NORMAL, FPF_ROMFONT }));
    if (!font) {
        close_all();
        return;
    }
    SetFont(&scr->RastPort, font);

    sbuf[0] = AllocScreenBuffer(scr, NULL, SB_SCREEN_BITMAP);
    sbuf[1] = AllocScreenBuffer(scr, NULL, SB_SCREEN_BITMAP);
    if (!sbuf[0] || !sbuf[1]) {
        close_all();
        return;
    }

    /* This NDK's ScreenBuffer carries no RastPort; bind our own to each
     * buffer bitmap. */
    for (cur_buf = 0; cur_buf < 2; ++cur_buf) {
        InitRastPort(&brp[cur_buf]);
        brp[cur_buf].BitMap = sbuf[cur_buf]->sb_BitMap;
        SetFont(&brp[cur_buf], font);
    }
    cur_buf = 0;
    ChangeScreenBuffer(scr, sbuf[0]);

    win = OpenWindowTags(NULL,
                         WA_CustomScreen, (Tag)scr,
                         WA_Left, 0,
                         WA_Top, 0,
                         WA_Width, SCREEN_PIXEL_WIDTH,
                         WA_Height, screen_h_px,
                         WA_Backdrop, TRUE,
                         WA_Borderless, TRUE,
                         WA_Activate, TRUE,
                         WA_RMBTrap, TRUE,
                         WA_IDCMP, IDCMP_RAWKEY | IDCMP_CLOSEWINDOW,
                         TAG_DONE);
    if (!win) {
        close_all();
        return;
    }

    screen_ready = 1;
    atexit(close_all);
}
