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
        int16_t px, py, dx, dy;
        uint32_t d;
        if (p->shape_id != current->shape_id ||
            !same_copy(p->x, current->x, w) || !same_copy(p->y, current->y, h)) continue;
        o = older_for(older, older_count, p, w, h);
        px = (int16_t)(p->x + (o ? wrap_delta(o->x, p->x, w) : 0));
        py = (int16_t)(p->y + (o ? wrap_delta(o->y, p->y, h) : 0));
        dx = wrap_delta(px, current->x, w);
        dy = wrap_delta(py, current->y, h);
        d = (uint32_t)((int32_t)dx * dx) + (uint32_t)((int32_t)dy * dy);
        if (!best || d < best_d) { best = p; best_d = d; }
    }
    return best_d > 16384UL ? 0 : best;
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
    out->x = (int16_t)(previous->x + ((int32_t)dx * elapsed) / interval);
    out->y = (int16_t)(previous->y + ((int32_t)dy * elapsed) / interval);
    if (ticks_per_second != 0) {
        out->angle = (uint16_t)(previous->angle +
            ((int32_t)previous->omega * elapsed) / ticks_per_second);
    }
}
