#ifndef BWC_AMIGA_VECTOR_OUTLINE_H
#define BWC_AMIGA_VECTOR_OUTLINE_H

#include <stdint.h>

/* Rectilinear silhouette tracing over an embedded shape cell bitmap.
 * Freestanding C99: no platform or OS includes, so it builds on the host
 * test harness exactly as it compiles into the Amiga target.
 *
 * The result is zero or more CLOSED rectilinear contours in world-unit
 * lattice coordinates (cell corners), covering every filled region:
 * connected areas, disconnected components and enclosed holes. Contours
 * are classified outer vs hole by even-odd containment (shoelace sign);
 * the renderer's fill policy draws outers with the shape pen, then holes
 * with the background pen, so the filled result equals the source bitmap. */

/* Bounds are sized for the embedded shape table (max width 5 => boundary
 * edges <= 4*25 = 100). VO_MAX_PTS must stay <= 255: indices into pts are
 * stored in uint8_t contour_start. */
#define VO_MAX_PTS      104
#define VO_MAX_CONTOURS 16  /* worst-case 5x5 checkerboard = 13 components */

typedef struct {
    int16_t x;
    int16_t y;
} vo_point;

typedef struct {
    vo_point pts[VO_MAX_PTS];
    uint8_t  contour_start[VO_MAX_CONTOURS];
    uint8_t  contour_len[VO_MAX_CONTOURS];
    uint8_t  is_hole[VO_MAX_CONTOURS];
    uint8_t  contour_count;
    uint8_t  truncated; /* set when geometry exceeded the bounds above */
} vo_outline;

/* Non-space byte marks a filled cell; out-of-bounds reads are empty. */
uint8_t vo_cell_filled(const uint8_t *cells, uint8_t width, uint8_t x, uint8_t y);

/* Trace `cells` (width*width row-major) into `out`. Returns 0 on success,
 * 1 if any contour/vertex bound was hit (partial contours still valid). */
int vo_trace(const uint8_t *cells, uint8_t width, vo_outline *out);

#endif /* BWC_AMIGA_VECTOR_OUTLINE_H */
