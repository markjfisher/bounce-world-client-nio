#include <conio.h>
#include "data.h"
#include "delay.h"

void cleanup_client(void)
{
    clrscr();
    pause(60); /* brief pause before returning to BASIC */
}
