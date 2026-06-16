#include <conio.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "app_errors.h"
#include "connection.h"
#include "convert_chars.h"
#include "data.h"
#include "debug.h"
#include "delay.h"
#include "hex_dump.h"
#include "shapes.h"

void parse_shape_records(const uint8_t *input)
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

uint8_t get_shape_count(void)
{
    create_command("x-shape-count");
    send_command();
    read_response_wait(app_data, 1);
    return app_data[0];
}

void read_and_parse_shapes_data(void)
{
    int n;

    memset(app_data, 0, APP_DATA_SIZE);
    create_command("x-shape-data");
    send_command();
    n = read_response_min(app_data, 1, APP_DATA_SIZE);

    if (n < 0) {
        err = (uint8_t)(-n);
        handle_err("shape data read");
    }

    parse_shape_records(app_data);
}

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

void get_shapes(void)
{
    uint8_t i;
    uint8_t x, y;
    char    tmp[6];

    cputsxy(0, 0, "Beginning parse of shapes data...");
    gotoxy(0, 21);
    cputs("call get_shape_count...  ");
    shape_count = get_shape_count();
    gotoxy(0, 21);
    cputs("got shape_count=");
    itoa(shape_count, tmp, 10);
    cputs(tmp);
    cputs("        ");
    memset(shapes, 0, sizeof(shapes));
    gotoxy(0, 21);
    cputs("call read_and_parse...   ");
    read_and_parse_shapes_data();
    gotoxy(0, 21);
    cputs("done read_and_parse      ");

    gotoxy(0, 1);
    cputs("Parsed shapes, count: ");
    itoa(shape_count, tmp, 10);
    cputs(tmp);

    for (i = 0; i < shape_count; i++) {
        x = (uint8_t)((i % 7) * 6);
        y = (uint8_t)(((uint8_t)(i / 7)) * 6 + 3);
        display_shape_data(i, x, y);
    }
}
