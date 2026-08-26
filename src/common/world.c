#include <conio.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
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
#include "keyboard.h"
#include "world.h"

static char *x_w_cmd = "x-w ";

void create_client_data_command(void)
{
    memset(client_data_cmd, 0, sizeof(client_data_cmd));
    strcpy(client_data_cmd, x_w_cmd);
    strcat(client_data_cmd, client_str);
    client_data_cmd_len = (uint8_t)strlen(client_data_cmd);
}

int16_t fetch_client_state(void)
{
#ifdef __AMIGA__
    static uint32_t last_fetch_completion_ticks = 0;
    uint32_t fetch_start = bwc_perf_ticks();
    uint32_t now;
    int16_t n;

    bwc_perf_fetch_begin();
    bwc_async_worker_phase = 21; /* worker: about to submit x-w */
#endif

    memset(app_data, 0, APP_DATA_SIZE);
    if (!request_client_data()) {
#ifdef __AMIGA__
        bwc_fetch_ticks_last = bwc_perf_ticks() - fetch_start;
#endif
        return 0;
    }
#ifdef __AMIGA__
    bwc_async_worker_phase = 22; /* worker: x-w submitted */
    bwc_async_worker_phase = 23; /* worker: about to read response */
    n = read_response_min(app_data, 1, APP_PAYLOAD_SIZE);
    bwc_async_worker_phase = 24; /* worker: read_response_min returned */
    now = bwc_perf_ticks();
    bwc_fetch_ticks_last = now - fetch_start;
    if (n > 0) {
        bwc_cnt_fetches++;
        bwc_fetch_interval_ticks_last = now - last_fetch_completion_ticks;
        last_fetch_completion_ticks = now;
    }
    return n;
#else
    return read_response_min(app_data, 1, APP_PAYLOAD_SIZE);
#endif
}

void get_world_state(void)
{
    uint8_t ws[14];

    create_command("x-ws");
    send_command();
    read_response_wait(ws, 14);

    /* Decode explicitly: the world-state globals must NOT be assumed
     * contiguous in memory. The previous raw 14-byte write into
     * &world_width relied on the compiler laying the block out without
     * gaps, which held on cc65 but not on m68k-amigaos-gcc, where the
     * spill zeroed shape_count. All targets are little-endian, so the
     * explicit LE decode is behaviour-preserving. */
    world_width      = (uint16_t)(ws[0] | ((uint16_t)ws[1] << 8));
    world_height     = (uint16_t)(ws[2] | ((uint16_t)ws[3] << 8));
    body_count       = (uint16_t)(ws[4] | ((uint16_t)ws[5] << 8));
    body_1           = ws[6];
    body_2           = ws[7];
    body_3           = ws[8];
    body_4           = ws[9];
    body_5           = ws[10];
    num_clients      = ws[11];
    world_is_frozen  = ws[12];
    world_is_wrapped = ws[13];
}

/* Fetch up to 512 bytes for all connected clients */
void get_world_clients(void)
{
    int n;
    uint16_t bytes_to_copy;
    uint8_t max_visible;

    memset(clients_buffer, ' ', sizeof(clients_buffer));
    clients_buffer_count = 0;

    create_command("x-who");
    send_command();
    n = read_response_prefix((uint8_t *)clients_buffer, sizeof(clients_buffer));
    if (n <= 0) {
        return;
    }

    max_visible = num_clients;
    if (max_visible > CLIENT_ROWS_MAX) {
        max_visible = CLIENT_ROWS_MAX;
    }

    bytes_to_copy = (uint16_t)n;
    if (bytes_to_copy > (uint16_t)(max_visible * CLIENT_NAME_WIDTH)) {
        bytes_to_copy = (uint16_t)(max_visible * CLIENT_NAME_WIDTH);
    }

    clients_buffer_count = (uint8_t)(bytes_to_copy / CLIENT_NAME_WIDTH);
}

void get_broadcast(void)
{
    int n;
    uint8_t copy_len;

    broadcast_message[0] = '\0';
    create_command("x-msg");
    send_command();
    n = read_response_min(app_data, 1, APP_PAYLOAD_SIZE);
    if (n <= 0) {
        return;
    }

    copy_len = (uint8_t)n;
    if (copy_len > BROADCAST_MAX_LEN) {
        copy_len = BROADCAST_MAX_LEN;
    }

    memcpy(broadcast_message, app_payload, copy_len);
    broadcast_message[copy_len] = '\0';
}

/* Process any pending server commands for this client */
void get_world_cmd(void)
{
    int  n;
    uint8_t i, cmd;
    bool fetch_broadcast = false;
    bool broadcast_enabled = false;

    create_command("x-cmd-get");
    append_command(client_str);
    send_command();
    n = read_response_min(app_data, 1, APP_PAYLOAD_SIZE);

    if (n > 0) {
        for (i = 0; i < (uint8_t)n; i++) {
            cmd = app_payload[i];
            switch (cmd) {
                case 1: is_darkmode = true;           set_screen_colours();  break;
                case 2: is_darkmode = false;          set_screen_colours();  break;
                case 3: is_showing_clients = true;                           break;
                case 4: is_showing_clients = false;                          break;
                case 5: fetch_broadcast = true; broadcast_enabled = true;    break;
                case 6: fetch_broadcast = true; broadcast_enabled = false;   break;
                case 7: is_showing_info = false;      toggle_info();         break;
                case 8: is_showing_info = true;       toggle_info();         break;
                default: break;
            }
        }

        if (fetch_broadcast) {
            get_broadcast();
            is_showing_broadcast = broadcast_enabled;
        }
    }
}
