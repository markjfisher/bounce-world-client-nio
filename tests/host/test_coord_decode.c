#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "shape_decode.h"

static int failures;

static void check_eq_int(int actual, int expected, const char *what)
{
    if (actual != expected) {
        printf("FAIL %s: got %d, expected %d\n", what, actual, expected);
        failures++;
    }
}

static void test_v2_basic(void)
{
    /* id=1 x=-1 y=2 | id=3 x=40 y=24 */
    const uint8_t payload[] = { 1, 0xFF, 0x02, 3, 40, 24 };
    ShapePos out[4];

    memset(out, 0xAA, sizeof(out));
    check_eq_int(bwc_decode_shapes(payload, sizeof(payload), 2, 2, out, 4), 2, "v2 count");
    check_eq_int(out[0].shape_id, 1, "v2 id0");
    check_eq_int(out[0].x, -1, "v2 negative x");
    check_eq_int(out[0].y, 2, "v2 y0");
    check_eq_int(out[1].shape_id, 3, "v2 id1");
    check_eq_int(out[1].x, 40, "v2 x1");
    check_eq_int(out[1].y, 24, "v2 y1");
}

static void test_v3_pixels(void)
{
    /* id=7 x=319 y=255 | id=9 x=-40 y=100 */
    const uint8_t payload[] = { 7, 0x3F, 0x01, 0xFF, 0x00, 9, 0xD8, 0xFF, 0x64, 0x00 };
    ShapePos out[4];

    memset(out, 0xAA, sizeof(out));
    check_eq_int(bwc_decode_shapes(payload, sizeof(payload), 2, 3, out, 4), 2, "v3 count");
    check_eq_int(out[0].shape_id, 7, "v3 id0");
    check_eq_int(out[0].x, 319, "v3 x > 127");
    check_eq_int(out[0].y, 255, "v3 y > 127");
    check_eq_int(out[1].shape_id, 9, "v3 id1");
    check_eq_int(out[1].x, -40, "v3 negative x");
    check_eq_int(out[1].y, 100, "v3 y");
}

static void test_v3_extremes(void)
{
    /* x = -32000 (0x8300 LE), y = 32000 (0x7D00 LE) */
    const uint8_t payload[] = { 5, 0x00, 0x83, 0x00, 0x7D };
    ShapePos out[2];

    memset(out, 0xAA, sizeof(out));
    check_eq_int(bwc_decode_shapes(payload, sizeof(payload), 1, 3, out, 2), 1, "v3 extreme count");
    check_eq_int(out[0].x, -32000, "v3 x -32000");
    check_eq_int(out[0].y, 32000, "v3 y 32000");
}

static void test_out_max_clamp(void)
{
    const uint8_t payload[15] = { 0 };
    ShapePos out[2];

    check_eq_int(bwc_decode_shapes(payload, sizeof(payload), 5, 3, out, 2), 2, "v3 clamped to out_max");
    check_eq_int(bwc_decode_shapes(payload, sizeof(payload), 5, 2, out, 2), 2, "v2 clamped to out_max");
    check_eq_int(bwc_decode_shapes(payload, sizeof(payload), 200, 2, out, 2), 2, "oversized count clamped");
}

static void test_degenerate(void)
{
    ShapePos out[2];
    uint8_t zero = 0;

    check_eq_int(bwc_decode_shapes(NULL, 16, 3, 3, out, 2), 0, "NULL payload");
    check_eq_int(bwc_decode_shapes(&zero, 16, 3, 3, NULL, 2), 0, "NULL out");
    check_eq_int(bwc_decode_shapes(&zero, 16, 3, 3, out, 0), 0, "zero out_max");
}

static void test_payload_len_bound(void)
{
    /* Claim 4 shapes but only provide bytes for 2 v3 records */
    const uint8_t payload[] = { 1, 0x01, 0x00, 0x02, 0x00,
                                2, 0x03, 0x00, 0x04, 0x00 };
    ShapePos out[8];

    memset(out, 0xAA, sizeof(out));
    check_eq_int(bwc_decode_shapes(payload, 7, 4, 3, out, 8), 1,
                 "v3 truncated mid-record drops partial record");
    check_eq_int(bwc_decode_shapes(payload, sizeof(payload), 4, 3, out, 8), 2,
                 "v3 full length decodes both records");
    check_eq_int(bwc_decode_shapes(payload, sizeof(payload), 4, 2, out, 8), 3,
                 "v2 stride fits more records in same bytes");
}

int main(void)
{
    test_v2_basic();
    test_v3_pixels();
    test_v3_extremes();
    test_out_max_clamp();
    test_degenerate();
    test_payload_len_bound();

    if (failures) {
        printf("%d failure(s)\n", failures);
        return 1;
    }
    printf("all coordinate decode tests passed\n");
    return 0;
}
