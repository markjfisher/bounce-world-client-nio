#ifndef BWC_AMIGA_GFX_RENDER_H
#define BWC_AMIGA_GFX_RENDER_H

#include <stdint.h>

/* Pixel-frame placeholder shape renderer (proportional filled rectangles).
 * Only provided by the amiga platform; referenced from common code when
 * wide-coordinate capabilities were negotiated at registration. */
void gfx_show_shape_px(uint8_t shape_id, int16_t center_x, int16_t center_y);

#endif /* BWC_AMIGA_GFX_RENDER_H */
