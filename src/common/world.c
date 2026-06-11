#include <conio.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "app_errors.h"
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
    memset(app_data, 0, APP_DATA_SIZE);
    request_client_data();
    return read_response_min((uint8_t *)app_data, 1, APP_DATA_SIZE);
}

void get_world_state(void)
{
    create_command("x-ws");
    send_command();
    read_response_wait((uint8_t *)&world_width, 14);
}

/* Fetch up to 512 bytes for all connected clients */
void get_world_clients(void)
{
    // memset(clients_buffer, 0, 512);
    // create_command("x-who");
    // send_command();
    // read_response_min((uint8_t *)clients_buffer, 1, 512);
}

void get_broadcast(void)
{
    // int n;
    // create_command("x-msg");
    // send_command();
    // n = read_response_min((uint8_t *)broadcast_message, 1, 119);
    // broadcast_message[n] = '\0';
}

/* Process any pending server commands for this client */
void get_world_cmd(void)
{
    int  n;
    uint8_t i, cmd;

    create_command("x-cmd-get");
    append_command(client_str);
    send_command();
    n = read_response_min((uint8_t *)app_data, 1, 256);

    if (n > 0) {
        for (i = 0; i < (uint8_t)n; i++) {
            cmd = app_data[i];
            switch (cmd) {
                case 1: is_darkmode = true;           set_screen_colours();  break;
                case 2: is_darkmode = false;          set_screen_colours();  break;
                case 3: is_showing_clients = true;                           break;
                case 4: is_showing_clients = false;                          break;
                case 5: get_broadcast(); is_showing_broadcast = true;        break;
                case 6: get_broadcast(); is_showing_broadcast = false;       break;
                case 7: is_showing_info = false;      toggle_info();         break;
                case 8: is_showing_info = true;       toggle_info();         break;
                default: break;
            }
        }
    }
}
