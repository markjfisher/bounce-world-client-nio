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
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "fujinet-nio.h"

#include "app_errors.h"
#include "connection.h"
#include "data.h"
#include "delay.h"
#include "screen.h"
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

    /* append LF (0x0A); avoid "\n" which cc65 maps to 0x9B on Atari */
    strcat((char *)cmd_tmp, "\x0A");
    len = (uint16_t)strlen((char *)cmd_tmp);

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
    uint16_t len     = (uint16_t)client_data_cmd_len;
    uint8_t attempt;

    /* append LF (0x0A) without mutating the cached command buffer */
    client_data_cmd[len]     = 0x0A;
    client_data_cmd[len + 1] = '\0';

    for (attempt = 0; attempt < 3; ++attempt) {
        written = 0;
        err = fn_write(server_handle, write_offset,
                       (const uint8_t *)client_data_cmd,
                       (uint16_t)(len + 1), &written);

        if (err == FN_OK) {
            break;
        }

        if (err != FN_ERR_IO && err != FN_ERR_TIMEOUT &&
            err != FN_ERR_BUSY && err != FN_ERR_NOT_READY) {
            break;
        }

        pause(3);
    }

    client_data_cmd[len] = '\0';
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

    result = fn_open(&server_handle, 0, server_url, FN_OPEN_STREAM_NO_PROBE);
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

static uint16_t packet_size_from_header(const uint8_t *buf)
{
    return (uint16_t)buf[0] | ((uint16_t)buf[1] << 8);
}

static void put_hex8(uint8_t value)
{
    static const char hex[] = "0123456789ABCDEF";

    cputc(hex[(value >> 4) & 0x0F]);
    cputc(hex[value & 0x0F]);
}

static void print_bad_packet_size(const uint8_t *buf, uint16_t packet_total, int16_t total)
{
    char tmp[7];

    gotoxy(0, 21);
    cputs("PKT h=");
    put_hex8(buf[0]);
    put_hex8(buf[1]);
    cputs(" size=");
    itoa((int)packet_total, tmp, 10);
    cputs(tmp);
    cputs(" got=");
    itoa((int)total, tmp, 10);
    cputs(tmp);
    cputs("   ");
}

static int16_t read_raw(uint8_t *buf, int16_t len)
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
            handle_err("read_raw");
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
 * read_response_wait – blocking read of a framed response with an exact
 * payload length.  The 2-byte little-endian packet size (including those
 * 2 bytes) is read and validated before payload bytes are stored at
 * payload_buf.
 */
int16_t read_response_wait(uint8_t *payload_buf, int16_t payload_len)
{
    uint8_t  header[PACKET_HEADER_SIZE];
    uint16_t packet_total;
    int16_t  n;

    n = read_raw(header, PACKET_HEADER_SIZE);
    if (n < PACKET_HEADER_SIZE) {
        return n;
    }

    packet_total = packet_size_from_header(header);
    if (packet_total != (uint16_t)(payload_len + PACKET_HEADER_SIZE)) {
        err = 1;
        handle_err("read_response_wait size");
        return -1;
    }

    return read_raw(payload_buf, payload_len);
}

/*
 * read_response_min – read a framed response into buf.
 *
 * The full packet (2-byte size prefix + payload) is stored in buf.
 * Payload begins at buf + PACKET_HEADER_SIZE; when buf is app_data, use
 * app_payload to access it.  Returns the payload length, or -1 on error.
 *
 * min_payload / max_payload refer to the payload only; max_payload must
 * leave room for the header in buf (e.g. APP_PAYLOAD_SIZE for app_data).
 */
int16_t read_response_min(uint8_t *buf, int16_t min_payload, int16_t max_payload)
{
    int16_t  total        = 0;
    uint16_t bytes_read   = 0;
    uint8_t  flags        = 0;
    uint8_t  result;
    int16_t  max_total    = (int16_t)(max_payload + PACKET_HEADER_SIZE);
    int16_t  need_total   = (int16_t)(min_payload + PACKET_HEADER_SIZE);
    uint16_t packet_total = 0;
    bool     have_header  = false;

    while (total < need_total) {
        result = fn_read(server_handle,
                         read_offset,
                         buf + total,
                         (uint16_t)(max_total - total),
                         &bytes_read,
                         &flags);

        if (result == FN_ERR_NOT_READY || result == FN_ERR_BUSY) {
            pause(3);
            continue;
        }

        if (result != FN_OK) {
            err = result;
            handle_err("read_response_min");
            return -1;
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

        if (total >= PACKET_HEADER_SIZE) {
            if (!have_header) {
                packet_total = packet_size_from_header(buf);
                if (packet_total < PACKET_HEADER_SIZE ||
                    packet_total > (uint16_t)max_total) {
                    err = 1;
                    print_bad_packet_size(buf, packet_total, total);
                    handle_err("read_response_min bad size");
                    return -1;
                }
                need_total   = (int16_t)packet_total;
                have_header  = true;
            }
            if (total >= need_total) {
                break;
            }
        }
    }

    if (!have_header) {
        return 0;
    }

    if (total < need_total) {
        return (int16_t)(total - PACKET_HEADER_SIZE);
    }

    if (packet_total != packet_size_from_header(buf)) {
        err = 1;
        handle_err("read_response_min mismatch");
        return -1;
    }

    return (int16_t)(packet_total - PACKET_HEADER_SIZE);
}

/* -----------------------------------------------------------------------
 * Client registration
 * --------------------------------------------------------------------- */

void send_client_data(void)
{
    char tmp[6];

    /* build "x-add-client name,2,screenX,screenY" */
    memset((char *)app_data, 0, 64);
    strcat((char *)app_data, name);
    strcat((char *)app_data, ",2,");    /* version 2 */
#ifdef __BBC__
    itoa(REG_SCREEN_WIDTH,  tmp, 10); strcat((char *)app_data, tmp);
    strcat((char *)app_data, ",");
    itoa(REG_SCREEN_HEIGHT, tmp, 10); strcat((char *)app_data, tmp);
#else
    itoa(SCREEN_WIDTH,  tmp, 10); strcat((char *)app_data, tmp);
    strcat((char *)app_data, ",");
    itoa(SCREEN_HEIGHT, tmp, 10); strcat((char *)app_data, tmp);
#endif

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

    create_client_data_command();
}
