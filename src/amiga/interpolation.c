#include <exec/tasks.h>
#include <clib/alib_protos.h>
#include <proto/exec.h>

#include <stdint.h>
#include <string.h>

#include "bwc_interpolation.h"
#include "bwc_interpolation_math.h"
#include "bwc_perf.h"
#include "data.h"
#include "keyboard.h"
#include "screen.h"
#include "status.h"
#include "world.h"

typedef struct {
    ShapePos shapes[SHAPE_POS_MAX];
    uint8_t count;
    uint8_t step;
    uint8_t status;
} bwc_snapshot_t;

static bwc_snapshot_t published;
static bwc_snapshot_t previous;
static bwc_snapshot_t older;
static volatile uint8_t worker_running;
static volatile uint8_t worker_alive;
static volatile uint8_t snapshot_ready;
volatile uint8_t bwc_async_worker_phase;
volatile uint8_t bwc_async_foreground_phase;
uint8_t bwc_interpolation_enabled = 1;
static uint32_t previous_ticks;
static uint32_t published_ticks;
static char command_queue[8];
static volatile uint8_t command_head, command_tail;

static uint8_t pop_command(char *command)
{
    uint8_t have_command;
    Forbid();
    have_command = command_head != command_tail;
    if (!have_command) {
        Permit();
        return 0;
    }
    *command = command_queue[command_tail];
    command_tail = (uint8_t)((command_tail + 1) % sizeof(command_queue));
    Permit();
    return 1;
}

static void bwc_fetch_worker(void)
{
    worker_alive = 1;
    while (worker_running) {
        char command;
        bwc_async_worker_phase = 2; /* worker: entering fetch_client_state */
        int16_t n = fetch_client_state();

        if (!worker_running) break;
        if (n > 0 && bwc_client_caps != 0) {
            bwc_snapshot_t next;

            bwc_async_worker_phase = 3; /* worker: response returned */

            next.count = bwc_decode_shapes(&app_payload[3],
                                            (uint16_t)(APP_DATA_SIZE - 3),
                                            app_payload[2], bwc_client_caps,
                                            next.shapes, SHAPE_POS_MAX);
            next.step = app_payload[0];
            next.status = app_payload[1];
            bwc_async_worker_phase = 4; /* worker: decoded */
            app_status = next.status;
            if (app_status != 0) handle_app_status();
            bwc_async_worker_phase = 5; /* worker: status processed */
            Forbid();
            older = previous;
            previous = published;
            previous_ticks = published_ticks;
            memcpy(&published, &next, sizeof(next));
            published_ticks = bwc_perf_ticks();
            snapshot_ready = 1;
            Permit();
            bwc_async_worker_phase = 6; /* worker: snapshot published */
        }
        while (pop_command(&command)) handle_kb_command(command);
    }
    worker_alive = 0;
}

uint8_t bwc_async_fetch_start(void)
{
    if (worker_running || bwc_client_caps == 0) return 0;
    memset(&published, 0, sizeof(published));
    memset(&previous, 0, sizeof(previous));
    memset(&older, 0, sizeof(older));
    snapshot_ready = 0;
    bwc_async_worker_phase = 0;
    bwc_async_foreground_phase = 0;
    command_head = command_tail = 0;
    worker_alive = 0;
    previous_ticks = 0;
    published_ticks = 0;
    worker_running = 1;
    if (CreateTask((STRPTR)"bwcn-fetch", 0, bwc_fetch_worker, 8192) == NULL) {
        worker_running = 0;
        return 0;
    }
    return 1;
}

void bwc_async_fetch_stop(void)
{
    worker_running = 0;
}

uint8_t bwc_async_fetch_alive(void)
{
    return worker_alive;
}

uint8_t bwc_async_fetch_active(void)
{
    return worker_running;
}

void bwc_interpolation_toggle(void)
{
    bwc_interpolation_enabled = (uint8_t)!bwc_interpolation_enabled;
}

void bwc_async_fetch_queue_command(char command)
{
    uint8_t next;
    Forbid();
    next = (uint8_t)((command_head + 1) % sizeof(command_queue));
    if (next != command_tail) {
        command_queue[command_head] = command;
        command_head = next;
    }
    Permit();
}

uint8_t bwc_async_fetch_copy(ShapePos *out, uint8_t out_max,
                             uint8_t *step_out, uint8_t *status_out)
{
    uint8_t count;
    uint8_t i;
    uint32_t elapsed;
    uint32_t interval;
    uint16_t wrap_width;
    uint16_t wrap_height;

    if (!snapshot_ready || out == NULL || out_max == 0) return 0;
    bwc_async_foreground_phase = 7; /* foreground: copying published snapshot */
    Forbid();
    count = published.count > out_max ? out_max : published.count;
    memcpy(out, published.shapes, (uint16_t)count * sizeof(ShapePos));
    /* Pixel positions are toroidal only when the server says this world
     * wraps. A solid-wall world must interpolate the direct coordinate
     * delta; folding it across 320x256 sends a normal bounce off-screen. */
    wrap_width = world_is_wrapped ? REG_SCREEN_WIDTH : 0;
    wrap_height = world_is_wrapped ? REG_SCREEN_HEIGHT : 0;
    interval = published_ticks - previous_ticks;
    elapsed = bwc_perf_ticks() - published_ticks;
    if (bwc_interpolation_enabled && previous_ticks != 0 && interval != 0 &&
        elapsed < interval) {
        for (i = 0; i < count; ++i) {
            ShapePos previous_image;

            if (bwc_interp_match_body_image(previous.shapes, previous.count,
                                             &out[i], wrap_width, wrap_height,
                                             &previous_image)) {
                /* previous_image has already been shifted into the same
                 * displayed tile as out[i], so blend a direct pixel segment. */
                bwc_interp_blend(&out[i], &previous_image, &out[i], elapsed,
                                 interval, bwc_perf_ticks_per_second(), 0, 0);
            }
        }
    }
    if (step_out != NULL) *step_out = published.step;
    if (status_out != NULL) *status_out = published.status;
    Permit();
    bwc_async_foreground_phase = 8; /* foreground: snapshot copy complete */
    return count;
}
