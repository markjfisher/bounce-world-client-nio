#include <stdint.h>

#include "delay.h"
#include "fetch_pacing.h"

/* The interval is rounded up, so configuration is always a rate ceiling.
 * PAL/8-bit targets use a 20 ms frame; Linux/MS-DOS use 16 ms. */
#if defined(__linux__) || defined(__MSDOS__)
#define BWC_FETCH_PACE_FRAME_MS 16U
#else
#define BWC_FETCH_PACE_FRAME_MS 20U
#endif

uint8_t bwc_fetch_pace_frames(uint16_t interval_ms)
{
    uint16_t frames = (uint16_t)((interval_ms + BWC_FETCH_PACE_FRAME_MS - 1U) /
                                 BWC_FETCH_PACE_FRAME_MS);

    return frames > 255U ? 255U : (uint8_t)frames;
}

void bwc_pace_fetch_interval(uint16_t interval_ms)
{
    pause(bwc_fetch_pace_frames(interval_ms));
}
