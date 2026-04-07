#include <conio.h>
#include "data.h"
#include "screen.h"

/*
 * Clear the play area only (top SCREEN_HEIGHT-2 rows when info is showing,
 * or the full screen otherwise).
 *
 * On BBC without hardware double-buffering we do a full clrscr.
 */
void playfield_clr(void)
{
    clrscr();
}
