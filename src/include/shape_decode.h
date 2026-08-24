#ifndef BWC_SHAPE_DECODE_H
#define BWC_SHAPE_DECODE_H

#include <stdint.h>

#ifndef BWC_CLIENT_VERSION
#define BWC_CLIENT_VERSION 2
#endif

/* Active client protocol version (set per target at build time) */
extern uint8_t bwc_client_version;

#define SHAPE_POS_MAX 84U

typedef struct {
    uint8_t  shape_id;
    int16_t  x;
    int16_t  y;
} ShapePos;

/* Decode world-state shape records from the server payload.
 * payload_len bounds the readable bytes; decoding stops at
 * min(count, payload_len/stride, out_max) records.
 * version < 3: 3 bytes/shape (id, signed x, signed y).
 * version >= 3: 5 bytes/shape (id, little-endian signed 16-bit x and y in
 * screen-pixel coordinates). Returns the number of decoded entries. */
uint8_t bwc_decode_shapes(const uint8_t *payload, uint16_t payload_len,
                          uint8_t count, uint8_t version,
                          ShapePos *out, uint8_t out_max);

#endif /* BWC_SHAPE_DECODE_H */
