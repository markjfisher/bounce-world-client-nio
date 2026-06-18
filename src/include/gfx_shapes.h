#ifndef BWC_GFX_SHAPES_H
#define BWC_GFX_SHAPES_H

#include <stdint.h>

/*
 * BBC MODE 7 teletext graphics shapes.
 *
 * Non-zero cells hold mosaic codes in &20-&7F.  Zero means transparent.
 * Column 0 of the playfield is the graphics colour control (17-23 in RAM).
 */

typedef struct {
    uint8_t        width;
    uint8_t        height;
    const uint8_t *cells; /* row-major, width * height bytes */
} GfxShapeDef;

extern const GfxShapeDef gfx_shapes[];
extern const uint8_t     gfx_shape_count;

#endif /* BWC_GFX_SHAPES_H */
