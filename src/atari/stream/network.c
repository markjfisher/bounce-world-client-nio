/* Application-owned TCP adapter. No changes to the AppStore transport. */
#include <string.h>
#include "fujinet-nio.h"
#include "fn_stream.h"
#include "delay.h"
#include "network.h"
#include "diagnostic.h"
static fn_stream_io_t stream;
static uint8_t failed;
/* Keep read-ahead independent of the library reply area: a write between
 * reads must not discard bytes belonging to the next application frame. */
static uint8_t pending[FN_STREAM_MAX];
static uint8_t pending_at, pending_size, pending_flags;
static void discard_pending(void) {
    pending_at = pending_size = pending_flags = 0;
}

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
    discard_pending();
    return result(fn_stream_init(&stream));
}
uint8_t bwc_stream_open(const char *url) {
    unsigned int n = strlen(url);
    uint8_t status;
    if (failed) return failed;
    if (n > FN_STREAM_MAX) return FN_ERR_URL_TOO_LONG;
    stream.length = (uint8_t)n;
    memcpy(stream.data, url, n);
    status = result(fn_stream_open(&stream));
    if (!status) discard_pending();
    return status;
}
uint8_t bwc_stream_close(void) {
    uint8_t status;
    /* Close only while the transaction sequence is known. Next init/open
     * replaces an abandoned connection, without pretending a write was undone. */
    if (failed) return failed;
    status = result(fn_stream_close(&stream));
    if (!status) discard_pending();
    return status;
}
uint8_t bwc_stream_read(uint8_t *buf, uint16_t capacity, uint16_t *actual, uint8_t *flags) {
    uint8_t status;
    *actual = 0; *flags = 0;
    if (failed) return failed;
    if (!capacity) return FN_ERR_INVALID;
    if (pending_at == pending_size) {
        if (pending_flags & FN_READ_EOF) { *flags = pending_flags; return FN_OK; }
        /* Read up to 120 immediately available bytes, not "wait for 120". */
        stream.length = FN_STREAM_MAX;
        bwc_diag_io_begin('R');
        status = result(fn_stream_read(&stream));
        bwc_diag_io_end(status);
        if (status) return status;
        pending_at = 0;
        pending_size = stream.actual;
        pending_flags = stream.flags;
        memcpy(pending, stream.received, pending_size);
    }
    *actual = pending_size - pending_at;
    if (*actual > capacity) *actual = capacity;
    memcpy(buf, pending + pending_at, *actual);
    pending_at += (uint8_t)*actual;
    *flags = pending_flags;
    if (pending_at < pending_size) {
        /* EOF belongs after all buffered data, including subsequent frames. */
        *flags = (uint8_t)((*flags & ~FN_READ_EOF) | FN_READ_MORE_AVAILABLE);
    }
    return FN_OK;
}
uint8_t bwc_stream_write(const uint8_t *buf, uint16_t length, uint16_t *actual) {
    uint8_t status, idle = 0;
    *actual = 0;
    if (failed) return failed;
    while (*actual < length) {
        stream.length = length - *actual > FN_STREAM_MAX ? FN_STREAM_MAX : (uint8_t)(length - *actual);
        memcpy(stream.data, buf + *actual, stream.length);
        bwc_diag_io_begin('W');
        status = result(fn_stream_write(&stream));
        bwc_diag_io_end(status);
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
