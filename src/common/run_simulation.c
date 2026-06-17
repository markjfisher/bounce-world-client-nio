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

    is_alt_screen        = 0;
    is_running_sim       = 1;
    is_showing_info      = 0;
    is_showing_broadcast = 0;
    is_showing_clients   = 0;
    flash_on_collision   = 0;
    play_sound_on_collision = 0;

    get_world_clients();

    while (is_running_sim) {
        // first 2 bytes are now the packet length, so we have to skip them
        n = fetch_client_state();

        /* Nothing received this round - re-poll */
        if (n == 3) {
            continue;
        }

        /* Handle any app-level status events from the server */
        app_status = app_data[3];
        if (app_status != 0) {
            handle_app_status();
        }

        /* Only redraw when the world step has advanced */
        new_step_id = app_data[2];
        if (new_step_id != current_step) {
            current_step = new_step_id;
            show_screen();
        }

        handle_kb();
    }

    /* Either errored or user quit - deregister from the server */
    disconnect_service();
}
