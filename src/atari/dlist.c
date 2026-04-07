#include <atari.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "debug.h"
#include "delay.h"
#include "dlist.h"

uint8_t txt_c1 = 0;
uint8_t txt_c2 = 0;
uint8_t txt_c3 = 0;

/* Returns a pointer to the screen memory address field inside the DLIST.
 * Walks past blank-line instructions to find the first LMS byte. */
uint8_t *get_dlist_screen_ptr(void)
{
    uint8_t *dlist = (uint8_t *)OS.sdlst;

    /* skip blank line instructions */
    while ((*dlist & 0x0F) == 0) {
        dlist++;
    }

    /* find the first LMS bit */
    while ((*dlist & 0x40) == 0) {
        dlist++;
    }

    /* skip the LMS instruction byte itself */
    dlist++;

    return dlist;
}

void setup_dli(void)
{
    uint8_t *dlist = (uint8_t *)OS.sdlst;

    ANTIC.nmien = 0x40; /* unset DLI bit while installing the handler */

    dlist[26] = 0x02 + 0x80; /* DLI on line 22 */
    dlist[27] = 0x02 + 0x80; /* DLI on line 23 */

    OS.vdslst = dli;
    enable_dli();
}
