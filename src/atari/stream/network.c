/* Application-owned TCP adapter. No changes to the AppStore transport. */
#include <string.h>
#include "fujinet-nio.h"
#include "fn_stream.h"
#include "delay.h"
#include "network.h"
static fn_stream_io_t stream;
static uint8_t failed;

/* A failed carrier operation may have succeeded remotely. Stop the session,
 * rather than allowing the caller's generic retries to change that request.
 * The library has already made three exact transaction attempts. */
static uint8_t result(uint8_t status) {
    if (status == FN_STREAM_IO_ERROR) status = FN_ERR_TRANSPORT;
    if (status == FN_STREAM_INVALID) status = FN_ERR_INVALID;
    if (status && status != FN_ERR_BUSY && status != FN_ERR_NOT_READY) failed = status;
    return status;
}
uint8_t bwc_stream_init(void) {
    failed = 0;
    return result(fn_stream_init(&stream));
}
uint8_t bwc_stream_open(const char *url) {
    unsigned int n = strlen(url);
    if (failed) return failed;
    if (n > FN_STREAM_MAX) return FN_ERR_URL_TOO_LONG;
    stream.length = (uint8_t)n;
    memcpy(stream.data, url, n);
    return result(fn_stream_open(&stream));
}
uint8_t bwc_stream_close(void) {
    /* Close only while the transaction sequence is known. Next init/open
     * replaces an abandoned connection, without pretending a write was undone. */
    if (failed) return failed;
    return result(fn_stream_close(&stream));
}
uint8_t bwc_stream_read(uint8_t *buf, uint16_t capacity, uint16_t *actual, uint8_t *flags) {
    uint8_t status;
    *actual = 0; *flags = 0;
    if (failed) return failed;
    if (!capacity) return FN_ERR_INVALID;
    stream.length = capacity > FN_STREAM_MAX ? FN_STREAM_MAX : (uint8_t)capacity;
    status = result(fn_stream_read(&stream));
    if (!status) {
        *actual = stream.actual; *flags = stream.flags;
        memcpy(buf, stream.received, stream.actual);
    }
    return status;
}
uint8_t bwc_stream_write(const uint8_t *buf, uint16_t length, uint16_t *actual) {
    uint8_t status, idle = 0;
    *actual = 0;
    if (failed) return failed;
    while (*actual < length) {
        stream.length = length - *actual > FN_STREAM_MAX ? FN_STREAM_MAX : (uint8_t)(length - *actual);
        memcpy(stream.data, buf + *actual, stream.length);
        status = result(fn_stream_write(&stream));
        if (status && status != FN_ERR_BUSY && status != FN_ERR_NOT_READY) return status;
        if (status || !stream.actual) {
            if (++idle == 120) { failed = FN_ERR_TIMEOUT; return failed; }
            network_retry_pause();
            continue;
        }
        idle = 0;
        *actual += stream.actual;
    }
    return FN_OK;
}
