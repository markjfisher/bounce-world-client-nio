#include <ctype.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <conio.h>

#ifdef __BBC__
#include <bbc.h>
#endif

#include "get_line.h"

void get_line(char *buf, uint8_t max_len)
{
    uint8_t c;
    uint8_t i = 0;
    uint8_t start_x = wherex();
    uint8_t cur_x;

    memset(buf, 0, max_len);
    cursor(1);

    do
    {
        c = (uint8_t)cgetc();
        cur_x = wherex();

        if (c == '\n')
        {
            c = CH_ENTER;
        }

        if (isprint((int)c))
        {
            gotox((uint8_t)(start_x + i));
            if (i == (uint8_t)(max_len - 1U))
            {
                revers(1);
                cursor(0);
            }
            else
            {
                revers(0);
                cursor(1);
            }
            cputc((char)c);
            buf[i] = (char)c;
            if (i < (uint8_t)(max_len - 1U))
            {
                ++i;
            }
        }
        else if (c == CH_CURS_LEFT || c == CH_DEL)
        {
            if (i != 0)
            {
                cur_x = wherex();

                gotox((uint8_t)(cur_x - 1U));
                if (i == (uint8_t)(max_len - 1U) && buf[i] != '\0')
                {
                    revers(0);
                    cursor(1);
                }
                else
                {
                    --i;
                }
                cputc(' ');
                gotox((uint8_t)(cur_x - 1U));
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
