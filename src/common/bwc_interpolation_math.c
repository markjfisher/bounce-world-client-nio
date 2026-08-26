#include "bwc_interpolation_math.h"

static int16_t wrap_delta(int16_t from, int16_t to, uint16_t extent)
{
    int16_t d = (int16_t)(to - from);
    if (extent != 0) {
        if (d > (int16_t)(extent / 2)) d = (int16_t)(d - extent);
        if (d < -(int16_t)(extent / 2)) d = (int16_t)(d + extent);
    }
    return d;
}

static uint8_t same_copy(int16_t a, int16_t b, uint16_t extent)
{
    if (extent == 0) return 1;
    if (a < 0 || b < 0) return a < 0 && b < 0;
    if (a >= (int16_t)extent || b >= (int16_t)extent)
        return a >= (int16_t)extent && b >= (int16_t)extent;
    return 1;
}

static uint8_t reverses(int16_t before, int16_t after)
{
    return (before < 0 && after > 0) || (before > 0 && after < 0);
}

static const ShapePos *older_for(const ShapePos *older, uint8_t older_count,
                                 const ShapePos *previous,
                                 uint16_t w, uint16_t h)
{
    const ShapePos *best = 0;
    uint32_t best_d = 0;
    uint8_t i;
    for (i = 0; i < older_count; ++i) {
        const ShapePos *p = &older[i];
        int16_t dx, dy;
        uint32_t d;
        if (p->shape_id != previous->shape_id ||
            !same_copy(p->x, previous->x, w) || !same_copy(p->y, previous->y, h)) continue;
        dx = wrap_delta(p->x, previous->x, w);
        dy = wrap_delta(p->y, previous->y, h);
        d = (uint32_t)((int32_t)dx * dx) + (uint32_t)((int32_t)dy * dy);
        if (!best || d < best_d) { best = p; best_d = d; }
    }
    return best;
}

const ShapePos *bwc_interp_match(const ShapePos *older, uint8_t older_count,
                                 const ShapePos *previous, uint8_t previous_count,
                                 const ShapePos *current, uint16_t w, uint16_t h)
{
    const ShapePos *best = 0;
    uint32_t best_d = 0;
    uint8_t i;
    for (i = 0; i < previous_count; ++i) {
        const ShapePos *p = &previous[i];
        const ShapePos *o;
        int16_t px, py, dx, dy, vx_before, vy_before, vx_after, vy_after;
        uint32_t d;
        if (p->shape_id != current->shape_id ||
            !same_copy(p->x, current->x, w) || !same_copy(p->y, current->y, h)) continue;
        o = older_for(older, older_count, p, w, h);
        vx_before = o ? wrap_delta(o->x, p->x, w) : 0;
        vy_before = o ? wrap_delta(o->y, p->y, h) : 0;
        vx_after = wrap_delta(p->x, current->x, w);
        vy_after = wrap_delta(p->y, current->y, h);

        /* A reversal is a bounce, not a line segment to interpolate through.
         * Present the current authoritative position until a continuous
         * history exists again. This also avoids choosing a wrap-copy on the
         * other side of a seam as a motion continuation. */
        if (o && (reverses(vx_before, vx_after) ||
                  reverses(vy_before, vy_after))) continue;

        px = (int16_t)(p->x + vx_before);
        py = (int16_t)(p->y + vy_before);
        dx = wrap_delta(px, current->x, w);
        dy = wrap_delta(py, current->y, h);
        d = (uint32_t)((int32_t)dx * dx) + (uint32_t)((int32_t)dy * dy);
        if (!best || d < best_d) { best = p; best_d = d; }
    }
    return best_d > 16384UL ? 0 : best;
}

uint8_t bwc_interp_match_body_image(const ShapePos *previous,
                                    uint8_t previous_count,
                                    const ShapePos *current,
                                    uint16_t w, uint16_t h,
                                    ShapePos *previous_image)
{
    uint32_t best_d = 0;
    uint8_t found = 0;
    uint8_t i;
    int8_t sx, sy;

    if (previous == 0 || current == 0 || previous_image == 0 ||
        current->body_id == 0) return 0;

    /* Every visible wrap image has the same logical body id. Choose the
     * prior representation (including its adjacent world translations)
     * closest to this particular current image. This is independent of the
     * server's packet/set order and never pairs another same-shaped body. */
    for (i = 0; i < previous_count; ++i) {
        const ShapePos *p = &previous[i];
        if (p->body_id != current->body_id) continue;
        for (sx = w ? -1 : 0; sx <= (w ? 1 : 0); ++sx) {
            for (sy = h ? -1 : 0; sy <= (h ? 1 : 0); ++sy) {
                ShapePos candidate = *p;
                int32_t dx, dy;
                uint32_t d;

                candidate.x = (int16_t)(candidate.x + (int32_t)sx * w);
                candidate.y = (int16_t)(candidate.y + (int32_t)sy * h);
                dx = (int32_t)candidate.x - current->x;
                dy = (int32_t)candidate.y - current->y;
                d = (uint32_t)(dx * dx) + (uint32_t)(dy * dy);
                if (!found || d < best_d) {
                    *previous_image = candidate;
                    best_d = d;
                    found = 1;
                }
            }
        }
    }
    return found;
}

void bwc_interp_blend(ShapePos *out, const ShapePos *previous,
                      const ShapePos *current, uint32_t elapsed,
                      uint32_t interval, uint32_t ticks_per_second,
                      uint16_t w, uint16_t h)
{
    int16_t dx, dy;
    if (elapsed >= interval || interval == 0) { *out = *current; return; }
    *out = *current;
    dx = wrap_delta(previous->x, current->x, w);
    dy = wrap_delta(previous->y, current->y, h);
    /* elapsed/interval are unsigned wall-clock values, but dx/dy and omega
     * are signed. Cast the time operands before multiplying: otherwise C
     * promotes a negative left/up delta to uint32_t and the result wraps far
     * off-screen. */
    out->x = (int16_t)(previous->x +
        ((int32_t)dx * (int32_t)elapsed) / (int32_t)interval);
    out->y = (int16_t)(previous->y +
        ((int32_t)dy * (int32_t)elapsed) / (int32_t)interval);
    if (ticks_per_second != 0) {
        out->angle = (uint16_t)(previous->angle +
            ((int32_t)previous->omega * (int32_t)elapsed) /
                (int32_t)ticks_per_second);
    }
}
