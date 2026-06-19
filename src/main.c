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

int main(void)
{
    get_info();
    clrscr();

    connect_service();
    send_client_data();
    show_shapes_preview();
    get_world_state();

    run_simulation();
    cleanup_client();
    cursor(1);

    return 0;
}
