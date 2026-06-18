#include <conio.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "app_errors.h"
#include "convert_chars.h"
#include "data.h"
#include "shapes.h"

#ifndef __BBC__
#include "embedded_shapes.h"

static void parse_shape_records(const uint8_t *input)
{
    uint8_t  i, j;
    uint8_t  dataLength;
    uint8_t  w;
    uint16_t current_offset = 0;
    const uint8_t *currentPos = input;

    memset(shapes_buffer, 0, SHAPES_BUFFER_SIZE);

    for (i = 0; i < shape_count; i++) {
        shapes[i].shape_id    = *currentPos++;
        w                     = *currentPos++;
        shapes[i].shape_width = w;

        dataLength = (uint8_t)(w * w);

        if (current_offset + dataLength <= SHAPES_BUFFER_SIZE) {
            shapes[i].shape_data = &shapes_buffer[current_offset];

            for (j = 0; j < dataLength; j++) {
                shapes_buffer[current_offset + j] = *currentPos++;
            }

            current_offset += dataLength;

            convert_chars(shapes[i].shape_data, dataLength);
            shapes[i].shape_data_len = dataLength;
        } else {
            err = 1;
            handle_err("shape buffer overflow");
            break;
        }
    }
}

void shapes_load_embedded(void)
{
    shape_count = embedded_shape_count;
    memset(shapes, 0, sizeof(shapes));
    parse_shape_records(embedded_shape_data);
}
#endif /* !__BBC__ */

void display_shape_data(uint8_t n, uint8_t x, uint8_t y)
{
    uint8_t i, j;
    uint8_t dataLength;
    uint8_t width;
    ShapeRecord shape;
    char *c;
    uint8_t actX;
    uint8_t actY;

    shape      = shapes[n];
    width      = shape.shape_width;
    dataLength = shape.shape_data_len;
    c          = (char *)shape.shape_data;

    for (i = 0; i < dataLength; i += width) {
        for (j = 0; j < width; ++j) {
            if (i + j < dataLength) {
                actX = x + j;
                actY = y + (i / width);
                cputcxy(actX, actY, *c);
                c++;
            }
        }
    }
}
