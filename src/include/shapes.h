#ifndef BWC_SHAPES_H
#define BWC_SHAPES_H

#include <stdint.h>

typedef struct {
    uint8_t  shape_id;
    uint8_t  shape_width;
    uint8_t  shape_data_len;
    uint8_t *shape_data;
} ShapeRecord;

typedef struct {
    uint8_t shapeId;
    int8_t  x;
    int8_t  y;
    uint8_t width;
} ShapeLocation;

extern ShapeRecord shapes[50];

void get_shapes(void);

#endif /* BWC_SHAPES_H */
