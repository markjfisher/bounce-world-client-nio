#ifndef BWC_SHAPE_DECODE_H
#define BWC_SHAPE_DECODE_H

#include <stdint.h>

/* Client capability bits, matching the pre-release server constants.
 * The wire mask has no fixed width; the client mask type is `unsigned`
 * so future bits fit. Unknown bits must be masked off, never acted on. */
typedef unsigned bwc_caps_t;

#define BWC_CAP_WIDE_COORDS 0x01U
#define BWC_CAP_ROTATION    0x02U /* reserved: rendering deferred */
#define BWC_CAP_BODY_ID     0x04U /* uint32 LE logical simulator id */

/* Caps negotiated for the current session (0 = legacy behaviour).
 * Set at registration time; never persisted. */
extern bwc_caps_t bwc_client_caps;

#define SHAPE_POS_MAX 84U

typedef struct {
    uint8_t  shape_id;
    int16_t  x;
    int16_t  y;
    uint16_t angle; /* reserved: decoded only with BWC_CAP_ROTATION */
    int16_t  omega; /* reserved: decoded only with BWC_CAP_ROTATION */
    uint32_t body_id; /* decoded only with BWC_CAP_BODY_ID */
} ShapePos;

/* Decode world-state shape records from the server payload.
 * payload_len bounds the readable bytes; decoding stops at
 * min(count, payload_len/stride, out_max) records.
 * Record stride depends on the negotiated caps mask:
 *   legacy (caps == 0):          3 bytes (id, signed x, signed y)
 *   BWC_CAP_WIDE_COORDS:         +2 bytes, stride 3 -> 5 (x/y widen from
 *                                uint8 to little-endian signed 16-bit
 *                                screen-pixel coordinates)
 *   BWC_CAP_ROTATION:            +4 bytes (angle uint16 LE, omega int16 LE)
 *   BWC_CAP_BODY_ID:             +4 bytes appended after rotation (uint32 LE)
 * Returns the number of decoded entries. */
uint8_t bwc_decode_shapes(const uint8_t *payload, uint16_t payload_len,
                          uint8_t count, bwc_caps_t caps,
                          ShapePos *out, uint8_t out_max);

#endif /* BWC_SHAPE_DECODE_H */
