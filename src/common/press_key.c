#include <conio.h>

#include "delay.h"
#include "world.h"

void press_key(void)
{
    chlinexy(6, 20, 28);
    revers(1);
    gotoxy(8, 21);
    cputs("Press a key to continue");
    revers(0);
    chlinexy(6, 22, 28);

    /* Poll the server while waiting for a keypress so we don't time out */
    while (kbhit() == 0) {
        fetch_client_state();
        pause(20);  /* ~1/3 second */
    }

    cgetc();  /* consume the key so it is not processed later */
}
