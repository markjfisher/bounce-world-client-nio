#include <conio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "connection.h"
#include "data.h"
#include "display.h"
#include "debug.h"
#include "delay.h"
#include "double_buffer.h"
#include "keyboard.h"
#ifdef __AMIGA__
#include "bwc_interpolation.h"
#include "bwc_perf.h"
#endif
#include "sound.h"
#include "status.h"
#include "world.h"

#ifdef __ATARI__
#include "dlist.h"
extern bool is_playing_collision;
#endif

void run_simulation(void)
{
    int n;
    uint8_t new_step_id;

    init_screen();
    cursor(0);

#ifdef __AMIGA__
    bwc_perf_init();
#endif

    is_alt_screen        = 0;
    is_running_sim       = 1;
    is_showing_info      = 0;
    is_showing_broadcast = 0;
    is_showing_clients   = 0;
    flash_on_collision   = 0;
    play_sound_on_collision = 0;

    get_world_clients();

#ifdef __AMIGA__
    if (bwc_async_fetch_start()) {
        ShapePos frame[SHAPE_POS_MAX];
        uint8_t frame_step;
        uint8_t frame_status;
        uint8_t frame_count;

        while (is_running_sim) {
            wait_vsync();
            bwc_async_foreground_phase = 1; /* foreground: display frame */
            frame_count = bwc_async_fetch_copy(frame, SHAPE_POS_MAX,
                                                &frame_step, &frame_status);
            {
                uint32_t render_start = bwc_perf_ticks();

                (void)frame_status;
                if (frame_count != 0) {
                current_step = frame_step;
                }
                /* A zero-shape snapshot is authoritative too: clear and
                 * present it rather than leaving the prior frame visible. */
                amiga_show_screen_shapes(frame, frame_count);
                bwc_render_ticks_last = bwc_perf_ticks() - render_start;
            }
            if (bwc_overlay_enabled) bwc_perf_overlay_draw();
            handle_kb();
        }
        bwc_async_fetch_stop();
        while (bwc_async_fetch_alive()) {
            wait_vsync();
        }
        disconnect_service();
        return;
    }
#endif

    while (is_running_sim) {
        n = fetch_client_state();

        /* Nothing received this round - re-poll */
        if (n <= 0) {
            continue;
        }

        /* Handle any app-level status events from the server */
        app_status = app_payload[1];
        if (app_status != 0) {
            handle_app_status();
        }

        /* Only redraw when the world step has advanced */
        new_step_id = app_payload[0];
        if (new_step_id != current_step) {
#ifdef __AMIGA__
            uint32_t render_start = bwc_perf_ticks();
#endif
            current_step = new_step_id;
            show_screen();
#ifdef __AMIGA__
            bwc_render_ticks_last = bwc_perf_ticks() - render_start;
            if (bwc_overlay_enabled) {
                bwc_perf_overlay_draw();
            }
#endif
        }

        handle_kb();
    }

    /* Either errored or user quit - deregister from the server */
    disconnect_service();
}
