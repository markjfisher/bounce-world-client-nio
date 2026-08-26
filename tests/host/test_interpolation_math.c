#include <assert.h>
#include <stdio.h>

#include "bwc_interpolation_math.h"

static ShapePos p(uint8_t id, int16_t x, int16_t y, uint16_t a, int16_t w)
{
    ShapePos v = { id, x, y, a, w };
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

    /* Two same-id candidates: nearest position is wrong, but motion from
     * the older packet predicts the copy that reaches x=30. */
    {
        ShapePos h[] = { p(2, 0, 0, 0, 0), p(2, 80, 0, 0, 0) };
        ShapePos q[] = { p(2, 10, 0, 0, 0), p(2, 40, 0, 0, 0) };
        ShapePos n = p(2, 30, 0, 0, 0);
        assert(bwc_interp_match(h, 2, q, 2, &n, 320, 200) == &q[0]);
    }

    puts("all interpolation math tests passed");
    return 0;
}
