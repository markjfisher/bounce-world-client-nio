#ifndef BWC_PERF_H
#define BWC_PERF_H

/*
 * Amiga-only fetch-latency instrumentation.
 *
 * Everything in this header is declared only for __AMIGA__; other targets
 * never see it and their code paths stay byte-identical. Counters are
 * bumped unconditionally on Amiga (a handful of increments per fetch, so
 * the overlay-off state stays effectively free while remaining instantly
 * readable the moment the overlay is toggled on).
 */

#ifdef __AMIGA__

#include <stdbool.h>
#include <stdint.h>

/* Wall-clock primitive backed by timer.device UNIT_MICROHZ (ReadEClock).
 * Granularity is a few microseconds, enough to resolve single-digit-ms
 * deltas. Returns 0 until bwc_perf_init() succeeds. */
void     bwc_perf_init(void);
uint32_t bwc_perf_ticks(void);
uint32_t bwc_perf_ticks_per_second(void);
uint32_t bwc_perf_delta_ms(uint32_t ticks);

/* Runtime overlay state (toggled from handle_kb) */
extern bool bwc_overlay_enabled;

/* Cumulative cost-attribution counters since program start */
extern uint32_t bwc_cnt_retry_pause;  /* NOT_READY/BUSY/zero-byte pauses      */
extern uint32_t bwc_cnt_write_retry;  /* x-w attempts that failed and retried */
extern uint32_t bwc_cnt_fn_read;      /* fn_read calls issued                 */
extern uint32_t bwc_cnt_bytes_read;   /* bytes actually served by fn_read     */
extern uint32_t bwc_cnt_fetches;      /* fetch_client_state calls with data   */

/* Per-fetch attribution. These are reset before each x-w request, so the
 * overlay reports the completed fetch rather than lifetime totals. */
extern uint32_t bwc_cnt_retry_pause_last;
extern uint32_t bwc_cnt_write_retry_last;
extern uint32_t bwc_cnt_fn_read_last;
extern uint32_t bwc_cnt_bytes_read_last;
extern uint32_t bwc_pause_ticks_last;
extern uint32_t bwc_read_ticks_last;
extern uint32_t bwc_write_ticks_last;

/* Last-completed durations in raw EClock ticks */
extern uint32_t bwc_fetch_ticks_last;
extern uint32_t bwc_render_ticks_last;
extern uint32_t bwc_fetch_interval_ticks_last;
extern uint32_t bwc_request_to_write_ticks_last;
extern uint32_t bwc_write_to_first_byte_ticks_last;
extern uint32_t bwc_first_byte_to_complete_ticks_last;
extern uint16_t bwc_write_exchanges_last;
extern uint16_t bwc_write_transport_retry_last;
extern uint16_t bwc_write_service_retry_last;
extern uint16_t bwc_write_zero_accepted_last;
extern uint16_t bwc_write_response_length_last;
extern uint8_t bwc_write_response_device_last;
extern uint8_t bwc_write_response_command_last;
extern uint8_t bwc_broker_stage_last;
extern uint8_t bwc_broker_result_last;
extern uint8_t bwc_write_status_last;
extern uint16_t bwc_write_bytes_last;
extern uint8_t bwc_read_called_last;
extern int16_t bwc_read_result_last;

void bwc_perf_overlay_toggle(void);
void bwc_perf_overlay_draw(void);
void bwc_perf_fetch_begin(void);
void bwc_perf_retry_pause(void);
void bwc_perf_write_complete(void);
void bwc_perf_first_byte(void);
void bwc_perf_fetch_complete(uint32_t now);
void bwc_perf_write_diagnostics(void);
void bwc_perf_write_result(uint8_t status, uint16_t written);
void bwc_perf_read_result(int16_t result);

#endif /* __AMIGA__ */

#endif /* BWC_PERF_H */
