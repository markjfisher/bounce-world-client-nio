#include <stdint.h>

#include "shape_decode.h"
uint8_t bwc_client_version = BWC_CLIENT_VERSION;

uint8_t bwc_decode_shapes(const uint8_t *payload, uint16_t payload_len,
                          uint8_t count, uint8_t version,
                          ShapePos *out, uint8_t out_max)
{
    uint8_t i;
    uint8_t n;
    uint16_t stride = (version >= 3U) ? 5U : 3U;
    uint16_t max_fit;

    if (!payload || !out || out_max == 0U || stride == 0U) {
        return 0;
    }

    n = count;
    max_fit = (uint16_t)(payload_len / stride);
    if (n > out_max) {
        n = out_max;
    }
    if ((uint16_t)n > max_fit) {
        n = (uint8_t)(max_fit > 0xFFU ? 0xFFU : max_fit);
    }

    if (version >= 3U) {
        for (i = 0; i < n; ++i) {
            const uint8_t *p = &payload[(uint16_t)i * 5U];
            out[i].shape_id = p[0];
            out[i].x = (int16_t)((uint16_t)p[1] | ((uint16_t)p[2] << 8));
            out[i].y = (int16_t)((uint16_t)p[3] | ((uint16_t)p[4] << 8));
        }
    } else {
        for (i = 0; i < n; ++i) {
            const uint8_t *p = &payload[(uint16_t)i * 3U];
            out[i].shape_id = p[0];
            out[i].x = (int8_t)p[1];
            out[i].y = (int8_t)p[2];
        }
    }

    return n;
}
