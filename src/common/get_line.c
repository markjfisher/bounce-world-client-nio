#ifdef __CC65__
#include <conio.h>
#include <ctype.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "get_line.h"

void get_line(char *buf, uint8_t max_len)
{
    uint8_t c;
    uint8_t i       = 0;
    uint8_t start_x = wherex();

    memset(buf, 0, max_len);
    cursor(1);

    do {
        c = cgetc();

        if (isprint(c)) {
            gotox(start_x + i);
            if (i == (max_len - 1)) {
                revers(1);
                cursor(0);
            } else {
                revers(0);
                cursor(1);
            }
            cputc((char)c);
            buf[i] = (char)c;
            if (i < max_len - 1) {
                i++;
            }
        } else if ((c == CH_CURS_LEFT) || (c == CH_DEL)) {
            if (i) {
                uint8_t cur_x = wherex();
                gotox(cur_x - 1);
                if (i == (max_len - 1) && buf[i] != '\0') {
                    revers(0);
                    cursor(1);
                } else {
                    --i;
                }
                cputc(' ');
                gotox(cur_x - 1);
                buf[i] = '\0';
            }
        }
    } while (c != CH_ENTER);

    cursor(0);
    revers(0);
    gotox(start_x);
    cputs(buf);
    cursor(0);
}
#endif /* __CC65__ */
