#include <conio.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

void hd(void *data, uint8_t size)
{
    uint8_t i = 0;
    uint8_t j = 0;
    uint8_t p = 0;
    uint8_t padding = 0;
    uint8_t c;
    char    hexStr[3];

    for (i = 0; i < size; i++) {
        memset(hexStr, 0, 3);
        itoa(*((uint8_t *)data + i), hexStr, 16);
        if (strlen(hexStr) < 2) {
            cputs("0");
        }
        cputs(hexStr);
        cputs(" ");

        if ((i + 1) % 8 == 0 || i == size - 1) {
            padding = (uint8_t)(((i + 1) % 8) ? (8 - (i + 1) % 8) : 0);
            for (p = 0; p < padding; p++) {
                cputs("   ");
            }
            cputs(" | ");
            for (j = (uint8_t)(i - (i % 8)); j <= i; j++) {
                c = *((uint8_t *)data + j);
                if (isprint(c)) {
                    cputc((char)c);
                } else {
                    cputc('.');
                }
            }
            cputs("\r\n");
        }
    }
}
