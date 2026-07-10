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
    uint8_t to_show  = clients_buffer_count;

    if (to_show > max_show) {
        to_show = max_show;
    }

    gotoxy(SCREEN_WIDTH - 11, 2);
    for (i = 0; i < 10; i++) {
        cputc(grid[0][i]);
    }

    for (i = 0; i < to_show; i++) {
        gotoxy(SCREEN_WIDTH - 11, 3 + i);
        cputc(CH_VLINE);
        for (j = 0; j < CLIENT_NAME_WIDTH; j++) {
            cputc(clients_buffer[(i * CLIENT_NAME_WIDTH) + j]);
        }
        cputc(CH_VLINE);
    }

    gotoxy(SCREEN_WIDTH - 11, 3 + to_show);
    for (i = 0; i < 10; i++) {
        cputc(grid[1][i]);
    }
}
