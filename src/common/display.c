#include <conio.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "broadcast.h"
#include "data.h"
#include "debug.h"
#include "delay.h"
#include "display.h"
#include "double_buffer.h"
#include "full_clr.h"
#include "hex_dump.h"
#include "keyboard.h"
#include "screen.h"
#include "shapes.h"
#include "playfield_clr.h"
#include "world.h"
#include "who.h"

#ifdef __ATARI__
#include <atari.h>
#include "dlist.h"
#include "vbi.h"
#include "fx.h"
#endif

#ifdef BWC_CUSTOM_CPUTC
void cputc_fast(char c);
void cincx(void);
void gotoxy_fast(uint8_t x, uint8_t y);
#endif

void set_screen_colours(void)
{
#ifdef __ATARI__
    if (is_darkmode || !is_showing_info) {
        txt_c1 = 0;
        txt_c2 = 0;
        txt_c3 = 0;
    } else {
        txt_c1 = INIT_COLOUR_1;
        txt_c2 = INIT_COLOUR_2;
        txt_c3 = INIT_COLOUR_3;
    }
#endif
}

void init_screen(void)
{
    clrscr();

#ifdef __ATARI__
    OS.color2 = 0;
    set_screen_colours();
    init_vbi();

    wait_vsync();
    setup_dli();
    wait_vsync();

    OS.noclik = 0xFF;

    dlist_scr_ptr   = get_dlist_screen_ptr();
    screen_mem_orig = (char *)((uint16_t)(dlist_scr_ptr[0]) |
                                  ((uint16_t)(dlist_scr_ptr[1]) << 8));
#endif
}

void show_shape(uint8_t shape_id, int8_t center_x, int8_t center_y)
{
    uint8_t i, j;
    int8_t  x, y;
    int8_t  start_x, start_y;
    ShapeRecord shape;
    uint8_t width;
    uint8_t iw = 0;
    char *data;
    bool first_x_in_row;
    char c;
    uint8_t max_y = SCREEN_HEIGHT;

    if (is_showing_info) {
        max_y = SCREEN_HEIGHT - 2;
    }

    shape  = shapes[shape_id];
    width  = shape.shape_width;
    data   = (char *)shape.shape_data;

    start_x = center_x - (int8_t)(width >> 1) - 1;
    start_y = center_y - (int8_t)(width >> 1) - 1;

    if ((width & 1) == 0) {
        start_x++;
        start_y++;
    }

    for (i = 0; i < width; ++i) {
        y = start_y + (int8_t)i;
        first_x_in_row = false;

        if (y >= 0 && y < (int8_t)max_y) {
            for (j = 0; j < width; ++j) {
                x = start_x + (int8_t)j;

                if (x >= 0 && x < SCREEN_WIDTH) {
                    if (!first_x_in_row) {
                        first_x_in_row = true;
#ifdef BWC_CUSTOM_CPUTC
                        gotoxy_fast((uint8_t)x, (uint8_t)y);
#else
                        gotoxy((uint8_t)x, (uint8_t)y);
#endif
                    }
                    c = data[iw + j];

#ifdef BWC_CUSTOM_CPUTC
                    cputc_fast(c);
#else
                    if (c != ' ') {
                        cputc(c);
                    } else {
                        gotox(wherex() + 1);
                    }
#endif
                } else if (first_x_in_row) {
                    break;
                }
            }
        }
        iw += width;
    }
}

void show_screen(void)
{
    uint8_t  i, shape_id;
    int8_t   x, y;
    uint16_t index            = 3;
    uint8_t  number_of_shapes = app_payload[2];

    swap_buffer();

    if (info_display_count < 2) {
        full_clr();
        if (is_showing_info) {
            show_info();
        }
        info_display_count++;
    } else {
        playfield_clr();
    }

    for (i = 0; i < number_of_shapes; ++i) {
        shape_id = app_payload[index++];
        x        = (int8_t)app_payload[index++];
        y        = (int8_t)app_payload[index++];
        show_shape(shape_id, x, y);
    }

    if (is_showing_clients) {
        show_clients();
    }

    if (is_showing_broadcast) {
        broadcast();
    }

    show_other_screen();
}
