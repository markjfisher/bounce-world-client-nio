#include <stdint.h>

#include "shape_decode.h"

bwc_caps_t bwc_client_caps = 0;

static uint16_t rd_le16(const uint8_t *p)
{
    return (uint16_t)p[0] | ((uint16_t)p[1] << 8);
}

uint8_t bwc_decode_shapes(const uint8_t *payload, uint16_t payload_len,
                          uint8_t count, bwc_caps_t caps,
                          ShapePos *out, uint8_t out_max)
{
    uint8_t i;
    uint8_t n;
    uint16_t stride = 3U;
    uint16_t max_fit;
    unsigned wide = caps & BWC_CAP_WIDE_COORDS;
    unsigned rot  = caps & BWC_CAP_ROTATION;

    if (!payload || !out || out_max == 0U) {
        return 0;
    }

    if (wide) {
        stride += 2U; /* x/y widen from uint8 to int16 LE */
    }
    if (rot) {
        stride += 4U; /* angle uint16 LE + omega int16 LE */
    }

    n = count;
    max_fit = (uint16_t)(payload_len / stride);
    if (n > out_max) {
        n = out_max;
    }
    if ((uint16_t)n > max_fit) {
        n = (uint8_t)(max_fit > 0xFFU ? 0xFFU : max_fit);
    }

    for (i = 0; i < n; ++i) {
        const uint8_t *p = &payload[(uint16_t)i * stride];

        out[i].shape_id = p[0];
        if (wide) {
            out[i].x = (int16_t)rd_le16(&p[1]);
            out[i].y = (int16_t)rd_le16(&p[3]);
        } else {
            out[i].x = (int8_t)p[1];
            out[i].y = (int8_t)p[2];
        }
        if (rot) {
            out[i].angle = rd_le16(&p[5]);
            out[i].omega = (int16_t)rd_le16(&p[7]);
        } else {
            out[i].angle = 0U;
            out[i].omega = 0;
        }
    }

    return n;
}
