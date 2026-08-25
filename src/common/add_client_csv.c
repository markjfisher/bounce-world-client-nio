#include <stdint.h>
#include <stdlib.h>

#include "add_client_csv.h"

/* Minimal bounded CSV builder for the x-add-client registration command.
 *
 * The writer carries its cursor/end as a single struct so helper calls
 * stay cheap on cc65 (BBC/Atari), whose MAIN memory area is tight enough
 * that naive per-argument helpers overflowed it. Decimal emission uses
 * the lightweight library itoa() there; other toolchains take the plain
 * divide-by-ten form. Numeric fields (protocol version, screen/world
 * dimensions) are small by construction, well inside int range on cc65.
 *
 * Hex emission is shift-only and shared. */

typedef struct {
    char *p;   /* write cursor */
    char *end; /* last byte kept free for the terminating NUL */
} csv_writer;

static int w_ch(csv_writer *w, char c)
{
    if (w->p >= w->end) {
        return 0;
    }
    *w->p++ = c;
    return 1;
}

static int w_text(csv_writer *w, const char *s)
{
    while (*s != '\0') {
        if (!w_ch(w, *s)) {
            return 0;
        }
        ++s;
    }
    return 1;
}

#if defined(__CC65__)
static int w_udec(csv_writer *w, unsigned v)
{
    char digits[8];

    digits[0] = '\0';
    itoa((int)v, digits, 10);
    return w_text(w, digits);
}
#else
static int w_udec(csv_writer *w, unsigned v)
{
    char digits[12]; /* enough for any 32-bit unsigned */
    uint8_t nd = 0;

    do {
        digits[nd++] = (char)('0' + (v % 10U));
        v /= 10U;
    } while (v != 0U);

    while (nd > 0U) {
        if (!w_ch(w, digits[--nd])) {
            return 0;
        }
    }
    return 1;
}
#endif

/* 0x-prefixed hex, minimal digits: the mask has no fixed width, so no
 * padding assumption is made (caps=1 -> "0x1", 0xDEADBEEF -> full width). */
static int w_uhex(csv_writer *w, unsigned v)
{
    static const char hex[] = "0123456789ABCDEF";
    char digits[2 * sizeof(unsigned)];
    uint8_t nd = 0;

    do {
        digits[nd++] = hex[v & 0xFU];
        v >>= 4;
    } while (v != 0U);

    if (!w_ch(w, '0') || !w_ch(w, 'x')) {
        return 0;
    }
    while (nd > 0U) {
        if (!w_ch(w, digits[--nd])) {
            return 0;
        }
    }
    return 1;
}

uint16_t bwc_build_add_client_csv(char *dst, uint16_t dst_cap,
                                  const char *name, unsigned version,
                                  unsigned screen_w, unsigned screen_h,
                                  unsigned world_w, unsigned world_h,
                                  unsigned caps)
{
    csv_writer w;
    unsigned nums[5];
    uint8_t k;

    if (!dst || dst_cap < 2U || !name || *name == '\0') {
        return 0;
    }

    w.p   = dst;
    w.end = dst + (dst_cap - 1U);

    nums[0] = version;
    nums[1] = screen_w;
    nums[2] = screen_h;
    nums[3] = world_w;
    nums[4] = world_h;

    if (!w_text(&w, name)) {
        return 0;
    }
    for (k = 0; k < 5U; ++k) {
        if (!w_ch(&w, ',') || !w_udec(&w, nums[k])) {
            return 0;
        }
    }
    if (caps != 0U) {
        if (!w_ch(&w, ',') || !w_uhex(&w, caps)) {
            return 0;
        }
    }

    *w.p = '\0';
    return (uint16_t)(w.p - dst);
}
