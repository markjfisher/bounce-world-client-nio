#ifndef BWC_LATENCY_TRACE_H
#define BWC_LATENCY_TRACE_H

/* Linux-only, opt-in fetch timing. Calls are compiled out by the shared
 * caller on every other target. Set BWC_LATENCY_TRACE=1 to enable it. */
void bwc_latency_trace_fetch_begin(void);
void bwc_latency_trace_write_complete(void);
void bwc_latency_trace_fetch_complete(int success);
void bwc_latency_trace_report(void);

#endif /* BWC_LATENCY_TRACE_H */
