#include <proto/graphics.h>

#include <stdint.h>

#include "delay.h"

void wait_vsync(void)
{
    WaitTOF();
}

void network_retry_pause(void)
{
    WaitTOF();
    WaitTOF();
}

void pause(uint8_t count)
{
    while (count-- > 0U) {
        wait_vsync();
    }
}
