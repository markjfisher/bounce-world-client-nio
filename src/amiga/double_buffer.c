#include "conio.h"

uint8_t is_alt_screen = 0;

void swap_buffer(void)
{
    is_alt_screen ^= 1U;
    amiga_conio_swap();
}

void show_other_screen(void)
{
    amiga_conio_present();
}
