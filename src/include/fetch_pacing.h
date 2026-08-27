#ifndef BWC_FETCH_PACING_H
#define BWC_FETCH_PACING_H

#include <stdint.h>

/* Convert a configured minimum snapshot interval to platform video frames. */
uint8_t bwc_fetch_pace_frames(uint16_t interval_ms);
void bwc_pace_fetch_interval(uint16_t interval_ms);

#endif /* BWC_FETCH_PACING_H */
