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
#include "shapes.h"
#include "shutdown.h"
#include "world.h"

int main(void)
{
    /* print app information and get the URL/name from the user */
    get_info();
    clrscr();

    /* make a persistent TCP connection to the server */
    connect_service();

    /* fetch shape data, register this client, get initial world state */
    get_shapes();
    send_client_data();
    get_world_state();

    /* run the simulation loop */
    run_simulation();

    /* cleanup screen/sound etc when simulation ends */
    cleanup_client();

    return 0;
}
