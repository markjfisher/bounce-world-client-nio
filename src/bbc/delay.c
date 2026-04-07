#include <stdint.h>
#include "delay.h"

/*
 * BBC Micro timing.
 *
 * cc65's BBC target does not expose a convenient vsync hook, so we use
 * a count-based busy loop as a portable approximation.
 *
 * wait_vsync() polls until the 5-byte centisecond timer at $292 (TIME)
 * has advanced by at least one tick (1/100 s), giving a ~10 ms cadence.
 * On a real BBC this is close enough for our purposes.
 */

/* Memory-mapped centisecond clock (5 bytes, big-endian at $291-$295) */
#define TIME_LO  (* (volatile uint8_t *) 0x0295)

void wait_vsync(void)
{
    uint8_t t = TIME_LO;
    while (TIME_LO == t) ; /* spin until the lowest byte changes */
}

void pause(uint8_t count)
{
    uint8_t i;
    for (i = 0; i < count; i++) {
        wait_vsync();
    }
}
