#define _POSIX_C_SOURCE 200809L

#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "bwc_latency_trace.h"

typedef struct {
    uint64_t total;
    uint64_t min;
    uint64_t max;
} latency_bucket;

static int trace_enabled = -1;
static uint64_t fetch_started;
static uint64_t write_completed;
static uint32_t fetch_count;
static latency_bucket write_bucket;
static latency_bucket read_bucket;
static latency_bucket total_bucket;

static uint64_t now_ns(void)
{
    struct timespec now;

    clock_gettime(CLOCK_MONOTONIC, &now);
    return (uint64_t)now.tv_sec * UINT64_C(1000000000) + (uint64_t)now.tv_nsec;
}

static int enabled(void)
{
    const char *setting;

    if (trace_enabled >= 0) {
        return trace_enabled;
    }
    setting = getenv("BWC_LATENCY_TRACE");
    trace_enabled = setting != NULL && setting[0] != '\0' &&
                    !(setting[0] == '0' && setting[1] == '\0');
    return trace_enabled;
}

static void record(latency_bucket *bucket, uint64_t elapsed)
{
    bucket->total += elapsed;
    if (bucket->min == 0 || elapsed < bucket->min) {
        bucket->min = elapsed;
    }
    if (elapsed > bucket->max) {
        bucket->max = elapsed;
    }
}

void bwc_latency_trace_fetch_begin(void)
{
    if (enabled()) {
        fetch_started = now_ns();
        write_completed = 0;
    }
}

void bwc_latency_trace_write_complete(void)
{
    if (enabled()) {
        write_completed = now_ns();
    }
}

void bwc_latency_trace_fetch_complete(int success)
{
    uint64_t completed;

    if (!enabled() || !success || fetch_started == 0 || write_completed == 0) {
        return;
    }

    completed = now_ns();
    record(&write_bucket, write_completed - fetch_started);
    record(&read_bucket, completed - write_completed);
    record(&total_bucket, completed - fetch_started);
    fetch_count++;
}

static void print_bucket(const char *name, const latency_bucket *bucket)
{
    fprintf(stderr, " %s=%.2f/%.2f/%.2f", name,
            (double)bucket->total / ((double)fetch_count * 1000000.0),
            (double)bucket->min / 1000000.0,
            (double)bucket->max / 1000000.0);
}

void bwc_latency_trace_report(void)
{
    if (!enabled() || fetch_count == 0) {
        return;
    }

    fprintf(stderr, "BWC latency (%" PRIu32 " successful x-w; avg/min/max ms):",
            fetch_count);
    print_bucket("write", &write_bucket);
    print_bucket("read", &read_bucket);
    print_bucket("total", &total_bucket);
    fputc('\n', stderr);
    fflush(stderr);
}
