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

/* Cohen-Sutherland clip of the segment (x0,y0)-(x1,y1) against the
 * rectangle [min_x,max_x] x [min_y,max_y] (inclusive). Returns 1 if any
 * part is visible, with the endpoints clipped in place so the drawn
 * segment has the true slope; 0 if the segment is entirely outside.
 * Wrapping worlds: the server sends one copy per visible wrap, with
 * centres that may sit off-screen - clip segments, never vertices. */
uint8_t vo_clip_segment(int32_t *x0, int32_t *y0, int32_t *x1, int32_t *y1,
                        int32_t min_x, int32_t min_y,
                        int32_t max_x, int32_t max_y);

#endif /* BWC_AMIGA_VECTOR_OUTLINE_H */
