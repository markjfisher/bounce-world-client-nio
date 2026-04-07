#ifndef BWC_DATA_H
#define BWC_DATA_H

#include <stdbool.h>
#include <stdint.h>
#include "fujinet-nio.h"
#include "shapes.h"

/* WARNING: if these change, need to update data.s too */
#define SHAPES_BUFFER_SIZE 512
/* room for 240 shapes * 3 bytes + header/status = 1024 bytes */
#define APP_DATA_SIZE      1024

/* TCP session handle for the server connection */
extern fn_handle_t server_handle;

/* Sequential read/write cursors - must advance cumulatively for the TCP stream */
extern uint32_t read_offset;
extern uint32_t write_offset;

/* endpoint to connect to, e.g. "tcp://host:port" */
extern char    server_url[80];
/* buffer for commands to send to the server */
extern uint8_t cmd_tmp[64];

/* scratch buffer for general network data */
extern uint8_t app_data[APP_DATA_SIZE];

/* buffer for shapes pixel data strings */
extern uint8_t shapes_buffer[SHAPES_BUFFER_SIZE];
extern char    clients_buffer[512];
extern char    broadcast_message[120];

extern char    name[9];
extern uint8_t name_pad;    /* pre-calculated: 9 - strlen(name) */

extern char    client_id;
extern char    client_str[8];
/* cached "x-w <id>" command for the per-frame fetch cycle */
extern char    client_data_cmd[10];
extern uint8_t client_data_cmd_len;

/* shape records: 50 * 5 bytes = 250 bytes */
extern ShapeRecord shapes[50];
extern uint8_t shape_count;

extern bool is_darkmode;
extern bool is_running_sim;
extern bool is_showing_info;
extern bool is_showing_clients;
extern bool is_showing_broadcast;
extern bool flash_on_collision;
extern bool play_sound_on_collision;

extern uint8_t current_step;
extern uint8_t app_status;

extern uint8_t info_display_count;

/* WORLD FLAGS/DATA - must stay contiguous in memory */
extern uint16_t world_width;
extern uint16_t world_height;
extern uint16_t body_count;
extern uint8_t  body_1;
extern uint8_t  body_2;
extern uint8_t  body_3;
extern uint8_t  body_4;
extern uint8_t  body_5;
extern uint8_t  num_clients;
extern uint8_t  world_is_frozen;
extern uint8_t  world_is_wrapped;

#endif /* BWC_DATA_H */
