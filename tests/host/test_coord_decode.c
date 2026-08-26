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

static void test_legacy_basic(void)
{
    /* id=1 x=-1 y=2 | id=3 x=40 y=24 (3-byte records, caps=0) */
    const uint8_t payload[] = { 1, 0xFF, 0x02, 3, 40, 24 };
    ShapePos out[4];

    memset(out, 0xAA, sizeof(out));
    check_eq_int(bwc_decode_shapes(payload, sizeof(payload), 2, 0, out, 4), 2,
                 "legacy count");
    check_eq_int(out[0].shape_id, 1, "legacy id0");
    check_eq_int(out[0].x, -1, "legacy negative x");
    check_eq_int(out[0].y, 2, "legacy y0");
    check_eq_int(out[1].shape_id, 3, "legacy id1");
    check_eq_int(out[1].x, 40, "legacy x1");
    check_eq_int(out[1].y, 24, "legacy y1");
    check_eq_int(out[0].angle, 0, "legacy angle reserved zero");
    check_eq_int(out[0].omega, 0, "legacy omega reserved zero");
}

static void test_wide_pixels(void)
{
    /* id=7 x=319 y=255 | id=9 x=-40 y=100 (5-byte records, WIDE_COORDS) */
    const uint8_t payload[] = { 7, 0x3F, 0x01, 0xFF, 0x00, 9, 0xD8, 0xFF, 0x64, 0x00 };
    ShapePos out[4];

    memset(out, 0xAA, sizeof(out));
    check_eq_int(bwc_decode_shapes(payload, sizeof(payload), 2,
                                   BWC_CAP_WIDE_COORDS, out, 4), 2,
                 "wide count");
    check_eq_int(out[0].shape_id, 7, "wide id0");
    check_eq_int(out[0].x, 319, "wide x > 127");
    check_eq_int(out[0].y, 255, "wide y > 127");
    check_eq_int(out[1].shape_id, 9, "wide id1");
    check_eq_int(out[1].x, -40, "wide negative x");
    check_eq_int(out[1].y, 100, "wide y");
    check_eq_int(out[0].angle, 0, "wide angle reserved zero");
    check_eq_int(out[0].omega, 0, "wide omega reserved zero");
}

static void test_wide_near_300(void)
{
    /* x encoded ~300 must stay ~300 (no uint8 wrap), y slightly negative */
    const uint8_t payload[] = { 4, 300 & 0xFF, (300 >> 8) & 0xFF, 0xFE, 0xFF };
    ShapePos out[2];

    memset(out, 0xAA, sizeof(out));
    check_eq_int(bwc_decode_shapes(payload, sizeof(payload), 1,
                                   BWC_CAP_WIDE_COORDS, out, 2), 1,
                 "wide ~300 count");
    check_eq_int(out[0].x, 300, "wide x ~300 no wrap");
    check_eq_int(out[0].y, -2, "wide y -2 edge straddle");
}

static void test_wide_extremes(void)
{
    /* x = -32000 (0x8300 LE), y = 32000 (0x7D00 LE) */
    const uint8_t payload[] = { 5, 0x00, 0x83, 0x00, 0x7D };
    ShapePos out[2];

    memset(out, 0xAA, sizeof(out));
    check_eq_int(bwc_decode_shapes(payload, sizeof(payload), 1,
                                   BWC_CAP_WIDE_COORDS, out, 2), 1,
                 "wide extreme count");
    check_eq_int(out[0].x, -32000, "wide x -32000");
    check_eq_int(out[0].y, 32000, "wide y 32000");
}

static void test_rotation_nine_byte(void)
{
    /* 9-byte record: id=6, x=160, y=120; angle_bits=16384 (0x4000 LE),
     * omega=-384 (0xFE80 LE) land in the reserved fields */
    const uint8_t payload[] = { 6, 160 & 0xFF, 160 >> 8, 120 & 0xFF, 120 >> 8,
                                0x00, 0x40, 0x80, 0xFE };
    ShapePos out[2];

    memset(out, 0xAA, sizeof(out));
    check_eq_int(bwc_decode_shapes(payload, sizeof(payload), 1,
                                   BWC_CAP_WIDE_COORDS | BWC_CAP_ROTATION,
                                   out, 2), 1, "rotation count");
    check_eq_int(out[0].shape_id, 6, "rotation id");
    check_eq_int(out[0].x, 160, "rotation x");
    check_eq_int(out[0].y, 120, "rotation y");
    check_eq_int(out[0].angle, 16384, "rotation angle 16384");
    check_eq_int(out[0].omega, -384, "rotation omega -384");

    /* Unknown high bits alongside a known bit are ignored by the decoder */
    memset(out, 0xAA, sizeof(out));
    check_eq_int(bwc_decode_shapes(payload, sizeof(payload), 1,
                                   BWC_CAP_WIDE_COORDS | BWC_CAP_ROTATION | 0xF0U,
                                   out, 2), 1,
                 "unknown extra bits ignored");
    check_eq_int(out[0].shape_id, 6, "unknown extra bits keep id");
    check_eq_int(out[0].x, 160, "unknown extra bits keep wide x");
    check_eq_int(out[0].y, 120, "unknown extra bits keep wide y");
    check_eq_int(out[0].angle, 16384, "unknown extra bits keep angle");
    check_eq_int(out[0].omega, -384, "unknown extra bits keep omega");
}

static void test_body_id(void)
{
    /* 9-byte record: id=6, x=-2, y=322, body id=0x12345678 LE. */
    const uint8_t payload[] = { 6, 0xFE, 0xFF, 0x42, 0x01,
                                0x78, 0x56, 0x34, 0x12 };
    ShapePos out[2];

    check_eq_int(bwc_decode_shapes(payload, sizeof(payload), 1,
                                   BWC_CAP_WIDE_COORDS | BWC_CAP_BODY_ID,
                                   out, 2), 1, "body id count");
    check_eq_int(out[0].x, -2, "body id x");
    check_eq_int(out[0].y, 322, "body id y");
    check_eq_int((int)out[0].body_id, 0x12345678, "body id LE");
}

static void test_out_max_clamp(void)
{
    const uint8_t payload[15] = { 0 };
    ShapePos out[2];

    check_eq_int(bwc_decode_shapes(payload, sizeof(payload), 5,
                                   BWC_CAP_WIDE_COORDS, out, 2), 2,
                 "wide clamped to out_max");
    check_eq_int(bwc_decode_shapes(payload, sizeof(payload), 5, 0, out, 2), 2,
                 "legacy clamped to out_max");
    check_eq_int(bwc_decode_shapes(payload, sizeof(payload), 200, 0, out, 2), 2,
                 "oversized count clamped");
}

static void test_degenerate(void)
{
    ShapePos out[2];
    uint8_t zero = 0;

    check_eq_int(bwc_decode_shapes(NULL, 16, 3, BWC_CAP_WIDE_COORDS, out, 2), 0,
                 "NULL payload");
    check_eq_int(bwc_decode_shapes(&zero, 16, 3, 0, NULL, 2), 0, "NULL out");
    check_eq_int(bwc_decode_shapes(&zero, 16, 3, 0, out, 0), 0, "zero out_max");
}

static void test_payload_len_bound(void)
{
    /* Claim 4 wide shapes but only provide bytes for 2 records */
    const uint8_t payload[] = { 1, 0x01, 0x00, 0x02, 0x00,
                                2, 0x03, 0x00, 0x04, 0x00 };
    ShapePos out[8];

    memset(out, 0xAA, sizeof(out));
    check_eq_int(bwc_decode_shapes(payload, 7, 4, BWC_CAP_WIDE_COORDS, out, 8),
                 1, "wide truncated mid-record drops partial record");
    check_eq_int(bwc_decode_shapes(payload, sizeof(payload), 4,
                                   BWC_CAP_WIDE_COORDS, out, 8), 2,
                 "wide full length decodes both records");
    check_eq_int(bwc_decode_shapes(payload, sizeof(payload), 4, 0, out, 8), 3,
                 "legacy stride fits more records in same bytes");

    /* 9-byte truncation: one full rotation record + 4 stray bytes */
    check_eq_int(bwc_decode_shapes(payload, sizeof(payload), 4,
                                   BWC_CAP_WIDE_COORDS | BWC_CAP_ROTATION,
                                   out, 8), 1,
                 "rotation truncated mid-record drops partial record");
}

int main(void)
{
    test_legacy_basic();
    test_wide_pixels();
    test_wide_near_300();
    test_wide_extremes();
    test_rotation_nine_byte();
    test_body_id();
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
