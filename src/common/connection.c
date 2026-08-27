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

#include "add_client_csv.h"
#include "app_errors.h"
#ifdef __AMIGA__
#include "bwc_perf.h"
#endif
#include "connection.h"
#include "data.h"
#include "delay.h"
#include "screen.h"
#include "shape_decode.h"
#include "world.h"

/* Protocol number reported in the registration CSV. It is informational
 * only: feature selection on the wire happens exclusively through the
 * capabilities bitmask below. */
#ifdef __AMIGA__
#define BWC_REGISTRATION_VERSION 3
#else
#define BWC_REGISTRATION_VERSION 2
#endif

/* Capabilities requested at registration. Only the Amiga target asks for
 * anything (WIDE_COORDS); every other target sends the legacy 6-field
 * form and keeps byte-identical behaviour. */
#ifdef __AMIGA__
// #define BWC_REQUESTED_CAPS ((bwc_caps_t)BWC_CAP_WIDE_COORDS) | ((bwc_caps_t)BWC_CAP_ROTATION)
#define BWC_REQUESTED_CAPS ((bwc_caps_t)BWC_CAP_WIDE_COORDS | \
                            (bwc_caps_t)BWC_CAP_BODY_ID)
#else
#define BWC_REQUESTED_CAPS ((bwc_caps_t)0)
#endif

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
uint8_t request_client_data(void)
{
    uint16_t written = 0;
    uint16_t len     = (uint16_t)client_data_cmd_len;
    uint8_t attempt;
#ifdef __AMIGA__
    uint32_t write_t0;
#endif

    /* append LF (0x0A) without mutating the cached command buffer */
    client_data_cmd[len]     = 0x0A;
    client_data_cmd[len + 1] = '\0';

    for (attempt = 0; attempt < 3; ++attempt) {
        written = 0;
#ifdef __AMIGA__
        write_t0 = bwc_perf_ticks();
#endif
        err = fn_write(server_handle, write_offset,
                       (const uint8_t *)client_data_cmd,
                       (uint16_t)(len + 1), &written);
#ifdef __AMIGA__
        bwc_write_ticks_last += bwc_perf_ticks() - write_t0;
#endif

        if (err == FN_OK) {
            break;
        }

        if (err != FN_ERR_IO && err != FN_ERR_TIMEOUT &&
            err != FN_ERR_BUSY && err != FN_ERR_NOT_READY) {
            break;
        }

#ifdef __AMIGA__
        bwc_cnt_write_retry++;
        bwc_cnt_write_retry_last++;
        bwc_cnt_retry_pause++;
        bwc_cnt_retry_pause_last++;
        bwc_perf_retry_pause();
#else
        network_retry_pause();
#endif
    }

    client_data_cmd[len] = '\0';
#ifdef __AMIGA__
    bwc_perf_write_diagnostics();
    bwc_perf_write_result(err, written);
#endif
    if (err != FN_OK) {
#ifdef BWC_IGNORE_TRANSIENT_CLIENT_IO
        if (err == FN_ERR_IO || err == FN_ERR_TIMEOUT ||
            err == FN_ERR_BUSY || err == FN_ERR_NOT_READY) {
            return 0;
        }
#endif
        handle_err("request_client_data");
        return 0;
    }
    write_offset += (uint32_t)written;
#ifdef __AMIGA__
    bwc_perf_write_complete();
#endif
    return 1;
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
#ifdef __AMIGA__
        bwc_cnt_fn_read++;
#endif
        result = fn_read(server_handle,
                         read_offset,
                         buf + total,
                         (uint16_t)(len - total),
                         &bytes_read,
                         &flags);

        if (result == FN_ERR_NOT_READY || result == FN_ERR_BUSY) {
#ifdef __AMIGA__
            bwc_cnt_retry_pause++;
            bwc_perf_retry_pause();
#else
            network_retry_pause();
#endif
            continue;
        }

        if (result != FN_OK) {
            err = result;
            handle_err("read_raw");
            return total;
        }

        if (bytes_read == 0 && (flags & FN_READ_EOF)) {
            err = FN_ERR_IO;
            handle_err("read_raw eof");
            return total;
        }

        if (bytes_read == 0) {
#ifdef __AMIGA__
            bwc_cnt_retry_pause++;
            bwc_perf_retry_pause();
#else
            network_retry_pause();
#endif
            continue;
        }

        /* The device may eagerly answer with more than the requested
         * length (get-minimum semantics: the surplus stays queued for the
         * next read). The stream position advances by what was actually
         * served, but this caller's buffer only received what it asked
         * for — never walk it past its end. */
#ifdef __AMIGA__
        bwc_cnt_bytes_read += bytes_read;
#endif
        read_offset += (uint32_t)bytes_read;
        if (bytes_read > (uint16_t)(len - total)) {
            bytes_read = (uint16_t)(len - total);
        }
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
    bool     eof_seen     = false;
#ifdef __AMIGA__
    uint32_t read_t0;
#endif

    while (total < need_total) {
#ifdef __AMIGA__
        bwc_cnt_fn_read++;
        read_t0 = bwc_perf_ticks();
#endif
        result = fn_read(server_handle,
                         read_offset,
                         buf + total,
                         (uint16_t)(max_total - total),
                         &bytes_read,
                         &flags);
#ifdef __AMIGA__
        bwc_read_ticks_last += bwc_perf_ticks() - read_t0;
        bwc_cnt_fn_read_last++;
#endif

        if (result == FN_ERR_NOT_READY || result == FN_ERR_BUSY) {
#ifdef __AMIGA__
            bwc_cnt_retry_pause++;
            bwc_cnt_retry_pause_last++;
            bwc_perf_retry_pause();
#else
            network_retry_pause();
#endif
            continue;
        }

        if (result != FN_OK) {
            err = result;
            handle_err("read_response_min");
            return -1;
        }

        if (bytes_read == 0 && (flags & FN_READ_EOF)) {
            err = FN_ERR_IO;
            handle_err("read_response_min eof");
            return -1;
        }

        if (bytes_read == 0) {
#ifdef __AMIGA__
            bwc_cnt_retry_pause++;
            bwc_cnt_retry_pause_last++;
            bwc_perf_retry_pause();
#else
            network_retry_pause();
#endif
            continue;
        }

        /* Same eager-over-return guard as read_raw: the stream advances by
         * what was served, the caller's buffer only by what it asked for. */
#ifdef __AMIGA__
        bwc_cnt_bytes_read += bytes_read;
        bwc_cnt_bytes_read_last += bytes_read;
        if (bytes_read != 0) {
            bwc_perf_first_byte();
        }
#endif
        read_offset += (uint32_t)bytes_read;
        if (bytes_read > (uint16_t)(max_total - total)) {
            bytes_read = (uint16_t)(max_total - total);
        }
        total       += (int16_t)bytes_read;

        if (flags & FN_READ_EOF) {
            eof_seen = true;
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
        if (eof_seen) {
            err = FN_ERR_IO;
            handle_err("read_response_min eof");
            return -1;
        }
        return (int16_t)(total - PACKET_HEADER_SIZE);
    }

    if (packet_total != packet_size_from_header(buf)) {
        err = 1;
        handle_err("read_response_min mismatch");
        return -1;
    }

    return (int16_t)(packet_total - PACKET_HEADER_SIZE);
}

/*
 * read_response_prefix – read a framed response, keep only the first
 * payload_capacity bytes, and drain the rest from the stream.
 *
 * This is used for responses such as x-who where the server can return more
 * rows than the client can display. It avoids a large permanent receive buffer.
 */
int16_t read_response_prefix(uint8_t *payload_buf, int16_t payload_capacity)
{
    uint8_t  header[PACKET_HEADER_SIZE];
    uint16_t packet_total;
    int16_t  payload_total;
    int16_t  keep_len;
    int16_t  remaining;
    int16_t  chunk;
    int16_t  n;

    n = read_raw(header, PACKET_HEADER_SIZE);
    if (n < PACKET_HEADER_SIZE) {
        return n;
    }

    packet_total = packet_size_from_header(header);
    if (packet_total < PACKET_HEADER_SIZE) {
        err = 1;
        handle_err("read_response_prefix size");
        return -1;
    }

    payload_total = (int16_t)(packet_total - PACKET_HEADER_SIZE);
    keep_len = payload_total;
    if (keep_len > payload_capacity) {
        keep_len = payload_capacity;
    }

    if (keep_len > 0) {
        n = read_raw(payload_buf, keep_len);
        if (n < keep_len) {
            return n;
        }
    }

    remaining = (int16_t)(payload_total - keep_len);
    while (remaining > 0) {
        chunk = remaining;
        if (chunk > APP_DATA_SIZE) {
            chunk = APP_DATA_SIZE;
        }
        n = read_raw(app_data, chunk);
        if (n < chunk) {
            return n;
        }
        remaining = (int16_t)(remaining - chunk);
    }

    return keep_len;
}

/* -----------------------------------------------------------------------
 * Client registration
 * --------------------------------------------------------------------- */

void send_client_data(void)
{
    uint16_t csv_len;

    /* build "x-add-client name,<version>,screenX,screenY,worldX,worldY"
     * plus an optional 7th caps field as 0x-prefixed hex text */
    memset((char *)app_data, 0, APP_DATA_SIZE);

    bwc_client_caps = BWC_REQUESTED_CAPS;
    csv_len = bwc_build_add_client_csv((char *)app_data, 64,
                                       name,
                                       (unsigned)BWC_REGISTRATION_VERSION,
                                       (unsigned)REG_SCREEN_WIDTH,
                                       (unsigned)REG_SCREEN_HEIGHT,
                                       (unsigned)REG_WORLD_WIDTH,
                                       (unsigned)REG_WORLD_HEIGHT,
                                       (unsigned)bwc_client_caps);
    if (csv_len == 0U || csv_len >= 64U) {
        err = 1;
        handle_err("build add-client");
        return;
    }

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
