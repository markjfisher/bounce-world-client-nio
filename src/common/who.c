#include <conio.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "data.h"
#include "screen.h"
#include "who.h"

/* for the corner/line chars */
#if defined(__ATARI__)
#include <atari.h>
#elif defined(__BBC__)
#include <bbc.h>
#elif defined(__CBM__)
#include <cbm.h>
#endif

static char grid[2][10] = {
    { CH_ULCORNER, CH_HLINE, CH_HLINE, CH_HLINE, CH_HLINE,
      CH_HLINE,    CH_HLINE, CH_HLINE, CH_HLINE, CH_URCORNER },
    { CH_LLCORNER, CH_HLINE, CH_HLINE, CH_HLINE, CH_HLINE,
      CH_HLINE,    CH_HLINE, CH_HLINE, CH_HLINE, CH_LRCORNER }
};

void show_clients(void)
{
    uint8_t i, j;
    uint8_t max_show = SCREEN_HEIGHT - 4;
    uint8_t to_show  = num_clients;

    if (to_show > max_show) {
        to_show = max_show;
    }

    /* top border */
    gotoxy(SCREEN_WIDTH - 11, 2);
    for (i = 0; i < 10; i++) {
        cputc(grid[0][i]);
    }

    /* client names (server pads each to 8 chars) */
    for (i = 0; i < to_show; i++) {
        gotoxy(SCREEN_WIDTH - 11, 3 + i);
        cputc(CH_VLINE);
        for (j = 0; j < 8; j++) {
            cputc(clients_buffer[(i << 3) + j]);
        }
        cputc(CH_VLINE);
    }

    /* bottom border */
    gotoxy(SCREEN_WIDTH - 11, 3 + num_clients);
    for (i = 0; i < 10; i++) {
        cputc(grid[1][i]);
    }
}
