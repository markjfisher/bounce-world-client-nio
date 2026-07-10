#include <conio.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "broadcast.h"
#include "data.h"
#include "screen.h"

/* for the corner chars */
#if defined(__ATARI__)
#include <atari.h>
#elif defined(__BBC__)
#include <bbc.h>
#elif defined(__CBM__)
#include <cbm.h>
#endif

#define MAX_WIDTH 22

void broadcast(void)
{
    char    lineBuffer[MAX_WIDTH + 1];
    uint8_t i;
    uint8_t lineLen = 0;
    uint8_t startCol;
    uint8_t startRow = 4;
    char   *msgPtr = broadcast_message;

    if (broadcast_message[0] == '\0') {
        return;
    }

    startCol = (SCREEN_WIDTH - MAX_WIDTH) / 2 - 1;

    gotoxy(startCol, startRow++);
    cputc(CH_ULCORNER);
    for (i = 0; i < MAX_WIDTH; i++) cputc(CH_HLINE);
    cputc(CH_URCORNER);

    memset(lineBuffer, ' ', MAX_WIDTH);
    lineBuffer[MAX_WIDTH] = '\0';

    while (*msgPtr != '\0') {
        if (*msgPtr == ' ' && lineLen == 0) {
            msgPtr++;
            continue;
        }

        if ((*msgPtr == ' ' && lineLen < MAX_WIDTH) || lineLen == MAX_WIDTH) {
            gotoxy(startCol, startRow++);
            cputc(CH_VLINE);
            cputs(lineBuffer);
            cputc(CH_VLINE);
            memset(lineBuffer, ' ', MAX_WIDTH);
            lineLen = 0;
            while (*msgPtr == ' ') msgPtr++;
            continue;
        }

        lineBuffer[lineLen++] = *msgPtr++;
    }

    if (lineLen > 0) {
        gotoxy(startCol, startRow++);
        cputc(CH_VLINE);
        cputs(lineBuffer);
        cputc(CH_VLINE);
    }

    gotoxy(startCol, startRow);
    cputc(CH_LLCORNER);
    for (i = 0; i < MAX_WIDTH; i++) cputc(CH_HLINE);
    cputc(CH_LRCORNER);
}
