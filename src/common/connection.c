/**
 * connection.c
 *
 * Server connection management using fujinet-nio-lib (fn_ API).
 *
 * Instead of device specs ("N1:tcp://...") and stateless calls, the NIO
 * library uses:
 *   - fn_handle_t  server_handle  : returned by fn_open()
 *   - uint32_t     read_offset    : cumulative bytes read from the stream
 *   - uint32_t     write_offset   : cumulative bytes written to the stream
 *
 * Both offsets must increase monotonically; the device uses them to
 * track position in the TCP stream.
 */

#include <conio.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "fujinet-nio.h"

#include "app_errors.h"
#include "connection.h"
#include "data.h"
#include "delay.h"
#include "hex_dump.h"
#include "press_key.h"
#include "screen.h"
#include "shapes.h"
#include "world.h"

/* -----------------------------------------------------------------------
 * Internal helpers
 * --------------------------------------------------------------------- */

void create_command(char *cmd)
{
    memset(cmd_tmp, 0, 64);
    strcpy((char *)cmd_tmp, cmd);
}

void append_command(char *cmd)
{
    strcat((char *)cmd_tmp, " ");
    strcat((char *)cmd_tmp, cmd);
}

/* Write cmd_tmp to the server and advance write_offset */
void send_command(void)
{
    uint16_t len     = (uint16_t)strlen((char *)cmd_tmp);
    uint16_t written = 0;

    err = fn_write(server_handle, write_offset,
                   (const uint8_t *)cmd_tmp, len, &written);
    if (err != FN_OK) {
        handle_err("send_command");
        return;
    }
    write_offset += (uint32_t)written;
}

/* Write the cached per-frame "x-w <id>" request */
void request_client_data(void)
{
    uint16_t written = 0;

    err = fn_write(server_handle, write_offset,
                   (const uint8_t *)client_data_cmd,
                   (uint16_t)client_data_cmd_len, &written);
    if (err != FN_OK) {
        handle_err("request_client_data");
        return;
    }
    write_offset += (uint32_t)written;
}

/* -----------------------------------------------------------------------
 * Connection lifecycle
 * --------------------------------------------------------------------- */

void connect_service(void)
{
    uint8_t result;

    result = fn_init();
    if (result != FN_OK) {
        err = result;
        handle_err("fn_init");
        return;
    }

    result = fn_open(&server_handle, 0, server_url, 0);
    if (result != FN_OK) {
        err = result;
        handle_err("connect");
        return;
    }

    /* Reset stream cursors for this new connection */
    read_offset  = 0;
    write_offset = 0;
}

void disconnect_service(void)
{
    create_command("close");
    append_command(client_str);
    send_command();
    fn_close(server_handle);
    server_handle = FN_INVALID_HANDLE;
}

/* -----------------------------------------------------------------------
 * Read helpers
 * --------------------------------------------------------------------- */

/*
 * read_response_wait – blocking read of exactly `len` bytes.
 *
 * Polls fn_read() until `len` cumulative bytes have been received.
 * fn_read returns FN_ERR_NOT_READY when no data is available; we pause
 * briefly and retry until we have enough.
 */
int16_t read_response_wait(uint8_t *buf, int16_t len)
{
    int16_t  total      = 0;
    uint16_t bytes_read = 0;
    uint8_t  flags      = 0;
    uint8_t  result;

    while (total < len) {
        result = fn_read(server_handle,
                         read_offset,
                         buf + total,
                         (uint16_t)(len - total),
                         &bytes_read,
                         &flags);

        if (result == FN_ERR_NOT_READY || result == FN_ERR_BUSY) {
            pause(3);   /* ~50 ms delay */
            continue;
        }

        if (result != FN_OK) {
            err = result;
            handle_err("read_response_wait");
            return total;
        }

        if (bytes_read == 0) {
            pause(3);
            continue;
        }

        read_offset += (uint32_t)bytes_read;
        total       += (int16_t)bytes_read;

        if (flags & FN_READ_EOF) {
            break;
        }
    }

    return total;
}

/*
 * read_response_min – non-blocking read of at least `min` bytes.
 *
 * Each call to fn_read returns whatever data is available; we loop until
 * the total reaches `min`.  On NOT_READY, we pause briefly (compensating
 * for network latency) rather than blocking the server.
 */
int16_t read_response_min(uint8_t *buf, int16_t min, int16_t max)
{
    int16_t  total      = 0;
    uint16_t bytes_read = 0;
    uint8_t  flags      = 0;
    uint8_t  result;

    while (total < min) {
        result = fn_read(server_handle,
                         read_offset,
                         buf + total,
                         (uint16_t)(max - total),
                         &bytes_read,
                         &flags);

        if (result == FN_ERR_NOT_READY || result == FN_ERR_BUSY) {
            pause(3);
            continue;
        }

        if (result != FN_OK) {
            err = result;
            handle_err("read_response_min");
            return total;
        }

        if (bytes_read == 0) {
            pause(3);
            continue;
        }

        read_offset += (uint32_t)bytes_read;
        total       += (int16_t)bytes_read;

        if (flags & FN_READ_EOF) {
            break;
        }
    }

    return total;
}

/* -----------------------------------------------------------------------
 * Client registration
 * --------------------------------------------------------------------- */

void send_client_data(void)
{
    char tmp[6];
    memset(tmp, 0, sizeof(tmp));

    /* build "x-add-client name,2,screenX,screenY" */
    memset((char *)app_data, 0, 64);
    strcat((char *)app_data, name);
    strcat((char *)app_data, ",2,");    /* version 2 */
    itoa(SCREEN_WIDTH,  tmp, 10); strcat((char *)app_data, tmp);
    strcat((char *)app_data, ",");
    itoa(SCREEN_HEIGHT, tmp, 10); strcat((char *)app_data, tmp);

    create_command("x-add-client");
    append_command((char *)app_data);
    send_command();
    read_response_wait((uint8_t *)&client_id, 1);

    if (client_id == 0) {
        err = 1;
        handle_err("bad client id");
    }

    memset(client_str, 0, 8);
    itoa(client_id, client_str, 10);

    cputsxy(10, 19, "Client ID: ");
    cputsxy(21, 19, client_str);

    /* cache the per-frame request command */
    create_client_data_command();

    press_key();
}
