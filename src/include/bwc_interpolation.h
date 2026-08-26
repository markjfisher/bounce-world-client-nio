#ifndef BWC_INTERPOLATION_H
#define BWC_INTERPOLATION_H

#ifdef __AMIGA__
#include <stdint.h>
#include "shape_decode.h"

uint8_t bwc_async_fetch_start(void);
void bwc_async_fetch_stop(void);
uint8_t bwc_async_fetch_alive(void);
uint8_t bwc_async_fetch_active(void);
void bwc_async_fetch_queue_command(char c);
uint8_t bwc_async_fetch_copy(ShapePos *out, uint8_t out_max,
                             uint8_t *step_out, uint8_t *status_out);
#endif

#endif
