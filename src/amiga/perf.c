/**
 * bwc_perf.c
 *
 * Amiga-only fetch-latency instrumentation: timer.device wall clock,
 * cost-attribution counters (storage lives here; the shared fetch/render
 * code bumps them under #ifdef __AMIGA__), and the keyboard-toggleable
 * on-screen overlay drawn on the reserved bottom rows.
 *
 * Compiled only into the Amiga target via the platform source wildcard;
 * no other target ever links this file.
 */

#include <conio.h>
#include <stdint.h>
#include <string.h>

#include <devices/timer.h>
#include <exec/devices.h>
#include <exec/io.h>
#include <exec/types.h>
#include <clib/alib_protos.h>
#include <proto/exec.h>
#include <proto/timer.h>

#include "bwc_perf.h"
#include "bwc_interpolation.h"
#include "delay.h"
#include "screen.h"

bool     bwc_overlay_enabled = false;

uint32_t bwc_cnt_retry_pause  = 0;
uint32_t bwc_cnt_write_retry  = 0;
uint32_t bwc_cnt_fn_read      = 0;
uint32_t bwc_cnt_bytes_read   = 0;
uint32_t bwc_cnt_fetches      = 0;

uint32_t bwc_cnt_retry_pause_last = 0;
uint32_t bwc_cnt_write_retry_last = 0;
uint32_t bwc_cnt_fn_read_last  = 0;
uint32_t bwc_cnt_bytes_read_last = 0;
uint32_t bwc_pause_ticks_last  = 0;
uint32_t bwc_read_ticks_last   = 0;
uint32_t bwc_write_ticks_last  = 0;

uint32_t bwc_fetch_ticks_last         = 0;
uint32_t bwc_render_ticks_last        = 0;
uint32_t bwc_fetch_interval_ticks_last = 0;

static struct MsgPort    *perf_port = NULL;
static struct timerequest *perf_req = NULL;
static uint32_t           perf_freq = 0;

/* The NDK declares TimerBase extern; GCC programs define it themselves. */
struct Device *TimerBase = NULL;

void bwc_perf_init(void)
{
    struct EClockVal eclock;

    /* Temporary first-frame diagnostic: the worker can stall before the
     * main loop has had a chance to process the O key. */
    bwc_overlay_enabled = true;
    bwc_cnt_retry_pause = 0;
    bwc_cnt_write_retry = 0;
    bwc_cnt_fn_read     = 0;
    bwc_cnt_bytes_read  = 0;
    bwc_cnt_fetches     = 0;
    bwc_perf_fetch_begin();

    if (perf_req != NULL) {
        return;
    }

    perf_port = CreatePort(NULL, 0);
    perf_req  = (struct timerequest *)CreateExtIO(perf_port,
                                                  sizeof(struct timerequest));
    if (perf_port == NULL || perf_req == NULL) {
        perf_freq = 0;
        return;
    }

    if (OpenDevice((STRPTR)"timer.device", UNIT_MICROHZ,
                   (struct IORequest *)perf_req, 0) != 0) {
        perf_freq = 0;
        return;
    }

    /* ReadEClock needs the library base of the opened timer device. */
    TimerBase = (struct Device *)perf_req->tr_node.io_Device;
    perf_freq = ReadEClock(&eclock);
}

void bwc_perf_fetch_begin(void)
{
    bwc_cnt_retry_pause_last = 0;
    bwc_cnt_write_retry_last = 0;
    bwc_cnt_fn_read_last     = 0;
    bwc_cnt_bytes_read_last  = 0;
    bwc_pause_ticks_last     = 0;
    bwc_read_ticks_last      = 0;
    bwc_write_ticks_last     = 0;
}

void bwc_perf_retry_pause(void)
{
    uint32_t start = bwc_perf_ticks();

    network_retry_pause();
    bwc_pause_ticks_last += bwc_perf_ticks() - start;
}

uint32_t bwc_perf_ticks(void)
{
    struct EClockVal eclock;

    if (perf_req == NULL || perf_freq == 0) {
        return 0;
    }
    ReadEClock(&eclock);
    return eclock.ev_lo;
}

uint32_t bwc_perf_ticks_per_second(void)
{
    return perf_freq;
}

uint32_t bwc_perf_delta_ms(uint32_t ticks)
{
    if (perf_freq == 0) {
        return 0;
    }
    return (uint32_t)(((unsigned long long)ticks * 1000ULL) / perf_freq);
}

void bwc_perf_overlay_toggle(void)
{
    bwc_overlay_enabled = !bwc_overlay_enabled;
}

/* Right-aligned decimal in a fixed field, capped so the overlay layout
 * can never overflow the 40-column text grid. */
static void print_padded(uint32_t v, uint8_t width)
{
    char buf[11];
    uint8_t len, i, cap;

    cap = width < sizeof(buf) - 1 ? width : (uint8_t)(sizeof(buf) - 1);
    if (v > 999999999UL) {
        v = 999999999UL;
    }
    utoa((unsigned int)v, buf, 10);
    len = (uint8_t)strlen(buf);
    if (len > cap) {
        cputs(buf + (len - cap));
        return;
    }
    for (i = len; i < cap; ++i) {
        cputc(' ');
    }
    cputs(buf);
}

void bwc_perf_overlay_draw(void)
{
    uint8_t y = SCREEN_HEIGHT - 4;
    uint32_t accounted = bwc_pause_ticks_last + bwc_read_ticks_last +
                         bwc_write_ticks_last;
    uint32_t other = bwc_fetch_ticks_last > accounted
                         ? bwc_fetch_ticks_last - accounted : 0;

    gotoxy(0, y);
    revers(0);
    cputs("RTY");
    print_padded(bwc_cnt_retry_pause_last, 3);
    cputs("/");
    print_padded(bwc_perf_delta_ms(bwc_pause_ticks_last), 3);
    cputs(" WR");
    print_padded(bwc_cnt_write_retry_last, 2);
    cputs(" RD");
    print_padded(bwc_cnt_fn_read_last, 2);
    cputs(" I");
    print_padded(bwc_perf_delta_ms(bwc_fetch_interval_ticks_last), 3);
    cputs("ms");

    gotoxy(0, (uint8_t)(y + 1));
    cputs("BYT");
    print_padded(bwc_cnt_bytes_read_last, 4);
    cputs(" F");
    print_padded(bwc_perf_delta_ms(bwc_fetch_ticks_last), 3);
    cputs(" R");
    print_padded(bwc_perf_delta_ms(bwc_render_ticks_last), 2);
    cputs(" IO");
    print_padded(bwc_perf_delta_ms(bwc_read_ticks_last), 3);
    cputs(" W");
    print_padded(bwc_perf_delta_ms(bwc_write_ticks_last), 2);
    cputs(" O");
    print_padded(bwc_perf_delta_ms(other), 2);

    gotoxy(0, (uint8_t)(y + 2));
    cputs("FG ");
    print_padded(bwc_async_foreground_phase, 2);
    cputs(" WK ");
    print_padded(bwc_async_worker_phase, 2);
    cputs(" A");
    cputc(bwc_async_fetch_alive() ? '1' : '0');
    cputs(" P");
    cputc(bwc_interpolation_enabled ? '1' : '0');
}
