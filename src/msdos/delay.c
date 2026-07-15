#include <dos.h>
#include <stdint.h>

#include "delay.h"

void wait_vsync(void)
{
    delay(16);
}

void network_retry_pause(void)
{
    delay(25);
}

void pause(uint8_t count)
{
    while (count-- > 0U) {
        wait_vsync();
    }
}
