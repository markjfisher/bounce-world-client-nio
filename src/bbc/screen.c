#include <stdint.h>

#include "screen.h"

uint8_t screen_buf[SCREEN_BUF_SIZE];
uint8_t gfx_colour = GFX_COLOUR_DEFAULT;

void gfx_cycle_colour(void)
{
    gfx_colour++;
    if (gfx_colour > GFX_COLOUR_LAST) {
        gfx_colour = GFX_COLOUR_FIRST;
    }
}

uint8_t gfx_get_colour(void)
{
    return gfx_colour;
}
