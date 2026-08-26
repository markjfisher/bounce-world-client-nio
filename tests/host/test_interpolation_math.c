#include <assert.h>
#include <stdio.h>

#include "bwc_interpolation_math.h"

static ShapePos p(uint8_t id, int16_t x, int16_t y, uint16_t a, int16_t w)
{
    ShapePos v = { id, x, y, a, w, 0 };
    return v;
}

static ShapePos body(uint8_t shape_id, uint32_t body_id, int16_t x, int16_t y)
{
    ShapePos v = p(shape_id, x, y, 0, 0);
    v.body_id = body_id;
    return v;
}

int main(void)
{
    ShapePos old[] = { p(1, 300, 10, 0, 0), p(1, -5, 10, 0, 0) };
    ShapePos prev[] = { p(1, 310, 10, 100, 100), p(1, -3, 10, 100, 100) };
    ShapePos cur = p(1, -1, 10, 200, 0);
    ShapePos out;

    /* The off-screen seam copy matches its own prior copy, not the body. */
    assert(bwc_interp_match(old, 2, prev, 2, &cur, 320, 200) == &prev[1]);
    bwc_interp_blend(&out, &prev[1], &cur, 50, 100, 1000, 320, 200);
    assert(out.x == -2 && out.y == 10);
    assert(out.angle == 105); /* omega-based advancement, not angle lerp */

    /* Stall freeze clamps to the newest authoritative snapshot. */
    bwc_interp_blend(&out, &prev[1], &cur, 150, 100, 1000, 320, 200);
    assert(out.x == cur.x && out.y == cur.y && out.angle == cur.angle);

    /* A solid-wall world has no seam: use the direct pixel delta rather
     * than folding a long move through the opposite edge. */
    {
        ShapePos wall_prev = p(1, 20, 10, 0, 0);
        ShapePos wall_cur = p(1, 300, 10, 0, 0);
        bwc_interp_blend(&out, &wall_prev, &wall_cur, 50, 100, 1000, 0, 0);
        assert(out.x == 160 && out.y == 10);
    }

    /* Signed deltas must remain signed while applying an unsigned EClock
     * elapsed value; left/up motion used to wrap to an off-screen position. */
    {
        ShapePos left_prev = p(1, 100, 100, 0, 0);
        ShapePos left_cur = p(1, 80, 80, 0, 0);
        bwc_interp_blend(&out, &left_prev, &left_cur, 50, 100, 1000, 0, 0);
        assert(out.x == 90 && out.y == 90);
    }

    /* A body crosses the right seam. Each current image chooses its nearest
     * equivalent prior image, not packet position or another same shape. */
    {
        ShapePos prior[] = { body(1, 42, 318, 40), body(1, 42, -2, 40),
                             body(1, 9, 100, 40) };
        ShapePos current_left = body(1, 42, 2, 40);
        ShapePos current_right = body(1, 42, 322, 40);
        ShapePos previous_image;

        assert(bwc_interp_match_body_image(prior, 3, &current_left,
                                            320, 256, &previous_image));
        assert(previous_image.x == -2 && previous_image.body_id == 42);
        assert(bwc_interp_match_body_image(prior, 3, &current_right,
                                            320, 256, &previous_image));
        assert(previous_image.x == 318 && previous_image.body_id == 42);
    }

    /* Two same-id candidates: nearest position is wrong, but motion from
     * the older packet predicts the copy that reaches x=30. */
    {
        ShapePos h[] = { p(2, 0, 0, 0, 0), p(2, 80, 0, 0, 0) };
        ShapePos q[] = { p(2, 10, 0, 0, 0), p(2, 40, 0, 0, 0) };
        ShapePos n = p(2, 30, 0, 0, 0);
        assert(bwc_interp_match(h, 2, q, 2, &n, 320, 200) == &q[0]);
    }

    /* A wall bounce reverses the x vector. It is a discontinuity, so the
     * renderer must use the current sample rather than blend through it. */
    {
        ShapePos h[] = { p(3, 300, 50, 0, 0) };
        ShapePos q[] = { p(3, 310, 50, 0, 0) };
        ShapePos n = p(3, 305, 50, 0, 0);
        assert(bwc_interp_match(h, 1, q, 1, &n, 320, 200) == NULL);
    }

    puts("all interpolation math tests passed");
    return 0;
}
