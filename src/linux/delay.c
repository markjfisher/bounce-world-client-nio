#define _POSIX_C_SOURCE 199309L

#include <time.h>
#include <stdint.h>

#include "delay.h"

static void sleep_milliseconds(long milliseconds)
{
    struct timespec ts;

    ts.tv_sec = milliseconds / 1000L;
    ts.tv_nsec = (milliseconds % 1000L) * 1000000L;
    nanosleep(&ts, NULL);
}

void wait_vsync(void)
{
    sleep_milliseconds(16L);
}

void network_retry_pause(void)
{
    sleep_milliseconds(25L);
}

void pause(uint8_t count)
{
    while (count-- > 0U) {
        wait_vsync();
    }
}
