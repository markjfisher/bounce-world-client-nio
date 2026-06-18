#include <stdint.h>
#include "double_buffer.h"
#include "screen.h"

uint8_t is_alt_screen = 0;
static uint8_t blit_rows = SCREEN_HEIGHT;

void swap_buffer(void)
{
    is_alt_screen ^= 1;
}

void set_blit_rows(uint8_t rows)
{
    blit_rows = rows;
}

void show_other_screen(void)
{
    screen_blit_rows(blit_rows);
}
