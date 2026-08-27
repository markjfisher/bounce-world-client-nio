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
#include "fujinet-nio.h"
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
uint32_t bwc_request_to_write_ticks_last = 0;
uint32_t bwc_write_to_first_byte_ticks_last = 0;
uint32_t bwc_first_byte_to_complete_ticks_last = 0;
uint16_t bwc_write_exchanges_last = 0;
uint16_t bwc_write_transport_retry_last = 0;
uint16_t bwc_write_service_retry_last = 0;
uint16_t bwc_write_zero_accepted_last = 0;
uint16_t bwc_write_response_length_last = 0;
uint8_t bwc_write_response_device_last = 0;
uint8_t bwc_write_response_command_last = 0;
uint8_t bwc_broker_stage_last = 0;
uint8_t bwc_broker_result_last = 0;
uint8_t bwc_write_status_last = 0xff;
uint16_t bwc_write_bytes_last = 0;
uint8_t bwc_read_called_last = 0;
int16_t bwc_read_result_last = 0;

static uint32_t fetch_started_ticks;
static uint32_t write_complete_ticks;
static uint32_t first_byte_ticks;

static struct MsgPort    *perf_port = NULL;
static struct timerequest *perf_req = NULL;
static uint32_t           perf_freq = 0;

/* The NDK declares TimerBase extern; GCC programs define it themselves. */
struct Device *TimerBase = NULL;

void bwc_perf_init(void)
{
    struct EClockVal eclock;

    /* Diagnostics remain available through the O key, but normal play must
     * not reserve screen space for the development overlay. */
    bwc_overlay_enabled = false;
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
    fetch_started_ticks          = bwc_perf_ticks();
    write_complete_ticks         = 0;
    first_byte_ticks             = 0;
    bwc_request_to_write_ticks_last = 0;
    bwc_write_to_first_byte_ticks_last = 0;
    bwc_first_byte_to_complete_ticks_last = 0;
    /* Write/result diagnostics are published by their respective completion
     * paths.  Do not blank them while the next asynchronous fetch is in
     * flight: at the foreground frame rate that transient placeholder was
     * usually what the overlay showed (W255/0, H0/0, B0/0), rather than a
     * completed FujiNet outcome. */
    bwc_cnt_retry_pause_last = 0;
    bwc_cnt_write_retry_last = 0;
    bwc_cnt_fn_read_last     = 0;
    bwc_cnt_bytes_read_last  = 0;
    bwc_pause_ticks_last     = 0;
    bwc_read_ticks_last      = 0;
    bwc_write_ticks_last     = 0;
}

void bwc_perf_write_diagnostics(void)
{
    fn_write_diagnostics_t diagnostics;

    fn_write_get_last_diagnostics(&diagnostics);
    bwc_write_exchanges_last = diagnostics.exchanges;
    bwc_write_transport_retry_last = diagnostics.transport_retry;
    bwc_write_service_retry_last = diagnostics.service_retry;
    bwc_write_zero_accepted_last = diagnostics.zero_accepted;
    bwc_write_response_length_last = diagnostics.response_length;
    bwc_write_response_device_last = diagnostics.response_device;
    bwc_write_response_command_last = diagnostics.response_command;
    fn_amiga_transport_last_broker_detail(&bwc_broker_stage_last,
                                          &bwc_broker_result_last);
}

void bwc_perf_write_result(uint8_t status, uint16_t written)
{
    bwc_write_status_last = status;
    bwc_write_bytes_last = written;
}

void bwc_perf_read_result(int16_t result)
{
    bwc_read_called_last = 1;
    bwc_read_result_last = result;
}

void bwc_perf_write_complete(void)
{
    write_complete_ticks = bwc_perf_ticks();
    bwc_request_to_write_ticks_last = write_complete_ticks - fetch_started_ticks;
}

void bwc_perf_first_byte(void)
{
    if (first_byte_ticks != 0) {
        return;
    }

    first_byte_ticks = bwc_perf_ticks();
    bwc_write_to_first_byte_ticks_last = first_byte_ticks - write_complete_ticks;
}

void bwc_perf_fetch_complete(uint32_t now)
{
    if (first_byte_ticks != 0) {
        bwc_first_byte_to_complete_ticks_last = now - first_byte_ticks;
    }
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
    uint16_t write_exchanges;
    uint16_t write_transport_retry;
    uint16_t write_service_retry;
    uint16_t write_zero_accepted;
    uint16_t write_response_length;
    uint8_t write_status;
    uint16_t write_bytes;
    uint8_t read_called;
    int16_t read_result;
    uint8_t response_device;
    uint8_t response_command;
    uint8_t broker_stage;
    uint8_t broker_result;

    /* The fetch worker resets and fills these fields independently of the
     * foreground renderer. Snapshot the related write outcome together so
     * one overlay frame never combines two different exchanges. */
    Forbid();
    write_exchanges = bwc_write_exchanges_last;
    write_transport_retry = bwc_write_transport_retry_last;
    write_service_retry = bwc_write_service_retry_last;
    write_zero_accepted = bwc_write_zero_accepted_last;
    write_response_length = bwc_write_response_length_last;
    write_status = bwc_write_status_last;
    write_bytes = bwc_write_bytes_last;
    read_called = bwc_read_called_last;
    read_result = bwc_read_result_last;
    response_device = bwc_write_response_device_last;
    response_command = bwc_write_response_command_last;
    broker_stage = bwc_broker_stage_last;
    broker_result = bwc_broker_result_last;
    Permit();

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
    cputs("LAT W");
    print_padded(bwc_perf_delta_ms(bwc_request_to_write_ticks_last), 3);
    cputs(" R");
    print_padded(bwc_perf_delta_ms(bwc_read_ticks_last), 3);
    cputs(" FB");
    print_padded(bwc_perf_delta_ms(bwc_write_to_first_byte_ticks_last), 3);
    cputs(" DR");
    print_padded(bwc_perf_delta_ms(bwc_first_byte_to_complete_ticks_last), 3);
    cputs(" T");
    print_padded(bwc_perf_delta_ms(bwc_fetch_ticks_last), 3);

    gotoxy(0, (uint8_t)(y + 2));
    cputs("WX");
    print_padded(write_exchanges, 3);
    cputs(" TR");
    print_padded(write_transport_retry, 2);
    cputs(" SR");
    print_padded(write_service_retry, 2);
    cputs(" ZA");
    print_padded(write_zero_accepted, 2);
    cputs(" RD");
    print_padded(bwc_cnt_fn_read_last, 2);
    cputs(" BY");
    print_padded(bwc_cnt_bytes_read_last, 3);

    gotoxy(0, (uint8_t)(y + 3));
    cputs("OUT W");
    print_padded(write_status, 3);
    cputs("/");
    print_padded(write_bytes, 3);
    cputs(" R");
    print_padded(read_called, 1);
    cputs("/");
    print_padded((uint16_t)read_result, 5);
    cputs(" H");
    print_padded(response_device, 2);
    cputs("/");
    print_padded(response_command, 2);
    cputs(" B");
    print_padded(broker_stage, 1);
    cputs("/");
    print_padded(broker_result, 3);

    (void)write_response_length;
}
