#include <cc65.h>
#include <conio.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "app_errors.h"

uint8_t err = 0;

void handle_err(char *reason)
{
    char tmp[6];
    if (err) {
        cursor(1);

        itoa((int) err, tmp, 10);

        gotoxy(0, 20);
        cputs("Error: ");
        cputs(reason);
        cputs(" : ");
        cputs(tmp);
        cputs("   ");
        gotoxy(0, wherey() + 1);

        if (doesclrscrafterexit()) {
            cgetc();
        }
        exit(1);
    }
}
