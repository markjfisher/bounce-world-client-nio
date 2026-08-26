#include <conio.h>
#include <screen.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "app_errors.h"
#ifdef __AMIGA__
#include "bwc_interpolation.h"
#include "bwc_perf.h"
#endif
#include "connection.h"
#include "data.h"
#include "debug.h"
#include "delay.h"
#include "display.h"
#include "world.h"

#ifdef __BBC__
#include "screen.h"
#endif

#ifdef __ATARI__
#include "dlist.h"
#endif

static void do_command(char *command)
{
    create_command(command);
    send_command();
    /* Discard the response byte but must read it to keep stream aligned */
    read_response_wait((uint8_t *)cmd_tmp, 1);
    get_world_state();
    info_display_count = 0;
}

static void add_body(uint8_t size)
{
    char size_string[4];
    itoa(size, size_string, 10);
    create_command("x-add-body");
    append_command(size_string);
    send_command();
    read_response_wait((uint8_t *)cmd_tmp, 1);
    get_world_state();
    info_display_count = 0;
}

void toggle_darkmode(void)
{
    is_darkmode = !is_darkmode;
    set_screen_colours();
}

void toggle_info(void)
{
    is_showing_info = !is_showing_info;
    set_screen_colours();
    if (!is_showing_broadcast) {
        info_display_count = 0;
    }
}

void handle_kb_command(char c)
{
    switch (c) {
        case '+': do_command("x-inc"); break;
        case '-': do_command("x-dec"); break;
        case 'F':
        case 'f': do_command("x-freeze"); break;

        case '1':
        case '2':
        case '3':
        case '4':
        case '5': add_body((uint8_t)(c - '0')); break;

        case 'R':
        case 'r': do_command("x-reset"); break;
        case 'I':
        case 'i': toggle_info(); break;
        case 'W':
        case 'w': is_showing_clients = !is_showing_clients; break;
        case 'Q':
        case 'q': is_running_sim = false; break;

#if defined(__BBC__)
        case 'C':
        case 'c': gfx_cycle_colour(); break;
#endif

#if defined(__ATARI__)
        case 'd': toggle_darkmode(); break;
        case 'l': flash_on_collision = !flash_on_collision; break;
#endif

#if defined(__AMIGA__)
        case 'V':
        case 'v':
            bwc_render_mode = (bwc_render_mode == RENDER_VECTOR)
                                  ? RENDER_BLOCK
                                  : RENDER_VECTOR;
            break;
        case 'O':
        case 'o':
            bwc_perf_overlay_toggle();
            break;
#endif

        default: break;
    }
}

void handle_kb(void)
{
    char c;
    if (kbhit() == 0) return;
    c = cgetc();
#ifdef __AMIGA__
    if (bwc_async_fetch_active()) {
        switch (c) {
            case '+': case '-': case 'F': case 'f': case 'R': case 'r':
            case '1': case '2': case '3': case '4': case '5':
                bwc_async_fetch_queue_command(c);
                return;
            default: break;
        }
    }
#endif
    handle_kb_command(c);
}
