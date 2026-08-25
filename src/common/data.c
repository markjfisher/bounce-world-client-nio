#include <stdbool.h>

#include "data.h"

/* TCP session handle returned by fn_open() */
fn_handle_t server_handle      = FN_INVALID_HANDLE;

/* Sequential read/write cursors for the open TCP session */
uint32_t    read_offset        = 0;
uint32_t    write_offset       = 0;

/* Network / app buffers */
// #ifdef __BBC__
// #pragma bss-name(push, "BIGBUF")
// #endif
char    server_url[80];
uint8_t app_data[APP_DATA_SIZE];
uint8_t shapes_buffer[SHAPES_BUFFER_SIZE];
char    clients_buffer[CLIENT_ROWS_MAX * CLIENT_NAME_WIDTH];
uint8_t clients_buffer_count;
char    broadcast_message[BROADCAST_MAX_LEN + 1];
uint8_t cmd_tmp[64];
char    name[9];
char    client_data_cmd[10];
// #ifdef __BBC__
// #pragma bss-name(pop)
// #endif
uint8_t name_pad;
uint8_t client_data_cmd_len;

/* Embedded shape records. */
ShapeRecord shapes[19];
uint8_t shape_count;

/* CLIENT INFO */
char    client_id;
char    client_str[8];

/* status byte from server */
uint8_t app_status;

/* WORLD FLAGS/DATA - must be contiguous in memory */
uint16_t world_width;
uint16_t world_height;
uint16_t body_count;
uint8_t  body_1;
uint8_t  body_2;
uint8_t  body_3;
uint8_t  body_4;
uint8_t  body_5;
uint8_t  num_clients;
uint8_t  world_is_frozen;
uint8_t  world_is_wrapped;

/* Simulation state */
bool    is_running_sim          = true;
uint8_t current_step            = 0xff;
uint8_t info_display_count      = 0;
bool    is_darkmode             = true;
bool    is_showing_info         = false;
bool    is_showing_clients      = false;
bool    is_showing_broadcast    = false;
enum render_mode bwc_render_mode = RENDER_VECTOR;
bool    flash_on_collision      = false;
bool    play_sound_on_collision = false;
