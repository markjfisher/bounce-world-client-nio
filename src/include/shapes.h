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

extern ShapeRecord shapes[19];

void get_shapes(void);
void parse_shape_records(const uint8_t *input);
uint8_t get_shape_count(void);
void read_and_parse_shapes_data(void);
void display_shape_data(uint8_t n, uint8_t x, uint8_t y);

#endif /* BWC_SHAPES_H */
