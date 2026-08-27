#include <stdint.h>
#include <stdio.h>

#include "fetch_pacing.h"

/* The unit under test calls pause only through bwc_pace_fetch_interval(),
 * not through the pure conversion function exercised here. */
void pause(uint8_t count)
{
    (void)count;
}

static int failures;

static void check_frames(uint16_t interval_ms, uint8_t expected)
{
    uint8_t actual = bwc_fetch_pace_frames(interval_ms);
    if (actual != expected) {
        printf("FAIL %u ms: got %u frames, expected %u\n",
               interval_ms, actual, expected);
        failures++;
    }
}

int main(void)
{
    check_frames(10U, 1U);
    check_frames(16U, 1U);
    check_frames(17U, 2U);
    check_frames(100U, 7U);
    check_frames(1000U, 63U);

    if (failures != 0) {
        return 1;
    }
    puts("all fetch pacing tests passed");
    return 0;
}
