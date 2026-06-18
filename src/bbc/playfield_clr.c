#include "data.h"
#include "screen.h"

void playfield_clr(void)
{
    uint8_t max_row = SCREEN_HEIGHT;

    if (is_showing_info) {
        max_row = SCREEN_HEIGHT - 2;
    }

    set_blit_rows(max_row);
    screen_playfield_clear(max_row);
}
