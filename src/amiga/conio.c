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
#include <string.h>

#include "screen.h"
#include "conio.h"

/* Classic NDK globals the proto/inline headers reference */
struct GfxBase *GfxBase = NULL;
struct IntuitionBase *IntuitionBase = NULL;

/* The Shell's default stack is far too small once show_screen() puts its
 * decode buffer and the render chain on it — the overflow trampled BSS
 * (shape_count, packet buffers). Ask for a real stack at startup. */
unsigned int __stack_size = 65536;

static struct Screen *scr;
static struct Window *win;
static struct TextFont *font;
static struct ScreenBuffer *sbuf[2];
static struct RastPort brp[2];
static uint16_t screen_h_px;
static uint8_t cur_buf;
static uint8_t swap_disabled;
static uint8_t cur_x, cur_y;
static uint8_t cur_revers;
static uint8_t cur_cursor_visible;
static uint8_t screen_ready;

/* Shadow copy of the text grid so the cursor block can be erased without
 * reading back the rastport. */
static uint8_t cell_ch[SCREEN_WIDTH * SCREEN_HEIGHT];
static uint8_t cell_rev[SCREEN_WIDTH * SCREEN_HEIGHT];
static uint8_t cursor_on_screen;
static uint8_t cursor_cell_x, cursor_cell_y;
static uint8_t cursor_saved_ch, cursor_saved_rev;

static void render_cell(uint8_t cx, uint8_t cy, uint8_t ch, uint8_t rev)
{
    struct RastPort *rp;

    /* Guard: out-of-grid positions (e.g. get_line editing past column 39)
     * must never reach the shadow arrays or the raster. */
    if (cx >= SCREEN_WIDTH || cy >= SCREEN_HEIGHT) {
        return;
    }
    rp = amiga_conio_draw_rp();
    if (!rp) {
        return;
    }
    /* Paint the full cell box explicitly, then the glyph on top. Avoids
     * JAM2/BPen background semantics, which proved unreliable here and left
     * cursor blocks un-erased. */
    SetAPen(rp, rev ? 1 : 0);
    RectFill(rp,
             (LONG)(cx * 8), (LONG)(cy * 8),
             (LONG)(cx * 8 + 7), (LONG)(cy * 8 + 7));
    if (ch != ' ') {
        SetAPen(rp, rev ? 0 : 1);
        SetDrMd(rp, JAM1);
        /* Baseline at row 6 of the 8px cell: ascenders and descenders both
         * stay inside the RectFill'd box (topaz descends 1px below
         * baseline). */
        Move(rp, (LONG)(cx * 8), (LONG)(cy * 8 + 6));
        Text(rp, (CONST_STRPTR)&ch, 1);
    }
}

static void cursor_erase(void)
{
    if (!cursor_on_screen) {
        return;
    }
    render_cell(cursor_cell_x, cursor_cell_y, cursor_saved_ch, cursor_saved_rev);
    cursor_on_screen = 0;
}

static void cursor_draw(void)
{
    struct RastPort *rp;

    if (!cur_cursor_visible || !screen_ready || cursor_on_screen) {
        return;
    }
    if (cur_x >= SCREEN_WIDTH || cur_y >= SCREEN_HEIGHT) {
        return;
    }
    rp = amiga_conio_draw_rp();
    if (!rp) {
        return;
    }
    cursor_cell_x = cur_x % SCREEN_WIDTH;
    cursor_cell_y = cur_y % SCREEN_HEIGHT;
    cursor_saved_ch = cell_ch[cursor_cell_y * SCREEN_WIDTH + cursor_cell_x];
    cursor_saved_rev = cell_rev[cursor_cell_y * SCREEN_WIDTH + cursor_cell_x];
    SetAPen(rp, 1);
    RectFill(rp,
             (LONG)(cursor_cell_x * 8), (LONG)(cursor_cell_y * 8),
             (LONG)(cursor_cell_x * 8 + 7), (LONG)(cursor_cell_y * 8 + 7));
    cursor_on_screen = 1;
}

void *amiga_conio_draw_rp(void)
{
    if (!screen_ready) {
        return NULL;
    }
    return &brp[cur_buf];
}

void amiga_conio_clear(void)
{
    struct RastPort *rp = amiga_conio_draw_rp();

    if (!rp) {
        return;
    }
    cursor_erase();
    SetAPen(rp, 0);
    RectFill(rp, 0, 0, SCREEN_PIXEL_WIDTH - 1, screen_h_px - 1);
    memset(cell_ch, ' ', sizeof(cell_ch));
    memset(cell_rev, 0, sizeof(cell_rev));
    cur_x = 0;
    cur_y = 0;
}

static void draw_char(char c)
{
    if (cur_x >= SCREEN_WIDTH || cur_y >= SCREEN_HEIGHT) {
        return;
    }
    if (c == '\n') {
        cur_x = 0;
        cur_y++;
    } else {
        render_cell(cur_x, cur_y, (uint8_t)c, cur_revers);
        cell_ch[cur_y * SCREEN_WIDTH + cur_x] = (uint8_t)c;
        cell_rev[cur_y * SCREEN_WIDTH + cur_x] = cur_revers;
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
    if (!screen_ready || swap_disabled) {
        return;
    }
    /* If the buffer swap is rejected, fall back to single-buffer drawing:
     * keep rendering into the visible bitmap rather than a never-shown
     * off-screen buffer. */
    if (!ChangeScreenBuffer(scr, sbuf[cur_buf])) {
        swap_disabled = 1;
        cur_buf = 0;
        ChangeScreenBuffer(scr, sbuf[0]);
    }
}

uint16_t amiga_conio_height(void)
{
    return screen_h_px;
}

void amiga_conio_swap(void)
{
    if (!swap_disabled) {
        cur_buf ^= 1U;
    }
}

void clrscr(void)
{
    amiga_conio_clear();
}

void gotoxy(uint8_t x, uint8_t y)
{
    cursor_erase();
    cur_x = x;
    cur_y = y;
    cursor_draw();
}

void gotox(uint8_t x)
{
    gotoxy(x, cur_y);
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
    cursor_erase();
    draw_char(c);
    cursor_draw();
}

void cputs(const char *s)
{
    cursor_erase();
    while (*s) {
        draw_char(*s++);
    }
    cursor_draw();
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
    uint8_t new_revers = on ? 1U : 0U;

    if (new_revers != cur_revers) {
        cursor_erase();
        cur_revers = new_revers;
        cursor_draw();
    }
}

void cursor(uint8_t on)
{
    uint8_t new_visible = on ? 1U : 0U;

    if (new_visible != cur_cursor_visible) {
        if (!new_visible) {
            cursor_erase();
        }
        cur_cursor_visible = new_visible;
        if (new_visible) {
            cursor_draw();
        }
    }
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
    uint8_t shift = (qualifier & (IEQUALIFIER_LSHIFT | IEQUALIFIER_RSHIFT)) ? 1U : 0U;
    char c = '\0';

    /* Ignore key-release events (IECODE_UP_PREFIX set) */
    if (code & IECODE_UP_PREFIX) {
        return '\0';
    }

    if (code <= 0x3F) {
        if (code >= 0x01 && code <= 0x0A && !shift) {
            c = "1234567890"[code - 0x01];
        } else if (code >= 0x10 && code <= 0x19) {
            c = row_q[code - 0x10];
        } else if (code >= 0x20 && code <= 0x28) {
            c = row_a[code - 0x20];
        } else if (code >= 0x31 && code <= 0x37) {
            /* Bottom row starts at 0x31: 0x30 is the international key */
            c = row_z[code - 0x31];
        } else {
            switch (code) {
                case 0x00: c = shift ? '~' : '`'; break;
                case 0x0B: c = shift ? '_' : '-'; break;
                case 0x0C: c = shift ? '+' : '='; break;
                case 0x0D: c = shift ? '|' : '\\'; break;
                case 0x1A: c = shift ? '{' : '['; break;
                case 0x1B: c = shift ? '}' : ']'; break;
                case 0x29: c = shift ? ':' : ';'; break;
                case 0x2A: c = shift ? '"' : '\''; break;
                case 0x38: c = shift ? '<' : ','; break;
                case 0x39: c = shift ? '>' : '.'; break;
                case 0x3A: c = shift ? '?' : '/'; break;
                default: break;
            }
        }

        if (c != '\0' && shift && c >= 'a' && c <= 'z') {
            c = (char)(c - 'a' + 'A');
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

static int16_t pushed_key = -1; /* decoded key from kbhit peek, -1 = none */

uint8_t kbhit(void)
{
    /* Consume messages until a mappable key-down is found (kept in
     * pushed_key for cgetc); key-ups and unmapped keys are discarded.
     * IDCMP messages must be answered with ReplyMsg — never re-queued. */
    if (!win) {
        return 0;
    }
    if (pushed_key >= 0) {
        return 1;
    }
    for (;;) {
        struct IntuiMessage *msg = (struct IntuiMessage *)GetMsg(win->UserPort);
        uint16_t class_;
        uint16_t code;
        uint16_t qualifier;

        if (!msg) {
            return 0;
        }
        class_ = msg->Class;
        code = msg->Code;
        qualifier = msg->Qualifier;
        ReplyMsg((struct Message *)msg);

        if (class_ == IDCMP_CLOSEWINDOW) {
            pushed_key = 'q';
            return 1;
        }
        if (class_ == IDCMP_RAWKEY) {
            char c = rawkey_to_ascii(code, qualifier);
            if (c != '\0') {
                pushed_key = c;
                return 1;
            }
        }
    }
}


char cgetc(void)
{
    struct IntuiMessage *msg;
    char result = '\0';

    if (pushed_key >= 0) {
        result = (char)pushed_key;
        pushed_key = -1;
        return result;
    }

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

    /* Explicit palette: don't rely on Intuition defaults for a custom
     * screen. 0 = background gray, 1 = text black, 2 = shape red. */
    SetRGB4(&scr->ViewPort, 0, 9, 9, 9);
    SetRGB4(&scr->ViewPort, 1, 0, 0, 0);
    SetRGB4(&scr->ViewPort, 2, 15, 3, 3);

    font = OpenFont((struct TextAttr *)&((struct TextAttr){ (STRPTR)"topaz.font", 8, FS_NORMAL, FPF_ROMFONT }));
    if (!font) {
        close_all();
        return;
    }
    SetFont(&scr->RastPort, font);

    sbuf[0] = AllocScreenBuffer(scr, NULL, SB_SCREEN_BITMAP);
    sbuf[1] = AllocScreenBuffer(scr, NULL, 0);
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
