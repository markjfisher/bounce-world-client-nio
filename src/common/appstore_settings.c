#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "appstore_settings.h"
#include "fujinet-nio.h"

#define APPSTORE_NAMESPACE "bounce-world-client"
#define APPSTORE_IO_SIZE 256

static uint8_t last_error = FN_OK;
static uint8_t appstore_buf[APPSTORE_IO_SIZE];
static fn_appstore_io_t appstore_io = { appstore_buf, sizeof(appstore_buf) };

bool appstore_read_setting(char *buffer, uint8_t max_len, const char *key)
{
    fn_appstore_read_t out;
    uint8_t result;

    if (buffer == 0 || max_len < 2) {
        last_error = FN_ERR_INVALID;
        return false;
    }

    buffer[0] = '\0';

    result = fn_appstore_read(&appstore_io,
                              APPSTORE_NAMESPACE,
                              key,
                              0,
                              (uint8_t *)buffer,
                              (uint16_t)(max_len - 1),
                              &out);
    last_error = result;
    if (result != FN_OK) {
        return false;
    }

    if ((out.flags & FN_APPSTORE_READ_EXISTS) == 0 || out.bytes_read == 0) {
        last_error = FN_OK;
        return false;
    }

    buffer[out.bytes_read] = '\0';
    return true;
}

bool appstore_write_setting(const char *buffer, const char *key)
{
    fn_appstore_write_t out;
    uint8_t result;
    uint16_t len;

    if (buffer == 0) {
        last_error = FN_ERR_INVALID;
        return false;
    }

    len = (uint16_t)strlen(buffer);
    result = fn_appstore_write(&appstore_io,
                               APPSTORE_NAMESPACE,
                               key,
                               0,
                               (const uint8_t *)buffer,
                               len,
                               &out);
    last_error = result;
    if (result != FN_OK) {
        return false;
    }

    if (out.bytes_written != len) {
        last_error = FN_ERR_IO;
        return false;
    }

    return true;
}

uint8_t appstore_last_error(void)
{
    return last_error;
}
