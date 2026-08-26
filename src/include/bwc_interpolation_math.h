#ifndef BWC_INTERPOLATION_MATH_H
#define BWC_INTERPOLATION_MATH_H

#include <stdint.h>
#include "shape_decode.h"

const ShapePos *bwc_interp_match(const ShapePos *older, uint8_t older_count,
                                 const ShapePos *previous, uint8_t previous_count,
                                 const ShapePos *current,
                                 uint16_t world_width, uint16_t world_height);
void bwc_interp_blend(ShapePos *out, const ShapePos *previous,
                      const ShapePos *current, uint32_t elapsed_ticks,
                      uint32_t interval_ticks, uint32_t ticks_per_second,
                      uint16_t world_width, uint16_t world_height);

#endif
