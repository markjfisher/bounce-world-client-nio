#include <stdint.h>
#include "double_buffer.h"

/*
 * BBC Micro double-buffering.
 *
 * The BBC has hardware support for shadow RAM / screen paging in *SHADOW and
 * HIMEM tricks, but that requires OS calls and is mode-dependent.
 *
 * For simplicity we use a software-only approach: swap_buffer() just flips
 * the flag (all drawing goes directly to the visible screen), and
 * show_other_screen() is a no-op.  This means there may be minor tearing on
 * a real BBC, but the protocol is correct.
 */

uint8_t is_alt_screen = 0;

void swap_buffer(void)
{
    is_alt_screen ^= 1;
}

void show_other_screen(void)
{
    /* Nothing required: BBC draws directly to the visible screen */
}
