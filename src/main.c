/**
 * bounce world client (NIO version)
 *
 * by Mark Fisher (c) 2024
 *
 * Uses fujinet-nio-lib for network communication.
 */

#include <cc65.h>
#include <conio.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "fujinet-nio.h"

#include "connection.h"
#include "data.h"
#include "hex_dump.h"
#include "get_info.h"
#include "run_simulation.h"
#include "shapes_preview.h"
#include "shutdown.h"
#include "world.h"
#ifdef __ATARI__
#include <atari.h>
#include "app_errors.h"
#include "sio_mode.h"

static void restore_sio_on_exit(void)
{
    ANTIC.nmien = 0x40; /* Never restore HSIO while our DLIs are active. */
    (void)bwc_restore_sio();
}
#endif

int main(void)
{
    get_info();
    clrscr();

    connect_service();
#ifdef __ATARI__
    atexit(restore_sio_on_exit);
    err = bwc_standard_sio();
    if (err != FN_OK) handle_err("set standard SIO");
#endif
    send_client_data();
    show_shapes_preview();
    get_world_state();

#ifdef __ATARI__
    /* Restore the VBI and keyboard IRQ even if a network error exits. */
    atexit(cleanup_client);
#endif
    run_simulation();
#ifndef __ATARI__
    cleanup_client();
#endif
    cursor(1);

    return 0;
}
