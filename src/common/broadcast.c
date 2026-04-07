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
    uint8_t i, lineLen, wordLen, startCol, startRow = 4;
    char   *msgPtr, *wordPtr;

    startCol = (SCREEN_WIDTH - MAX_WIDTH) / 2 - 1;

    gotoxy(startCol, startRow++);
    cputc(CH_ULCORNER);
    for (i = 0; i < MAX_WIDTH; i++) cputc(CH_HLINE);
    cputc(CH_URCORNER);

    msgPtr = broadcast_message;
    lineBuffer[0] = '\0';
    lineLen = 0;

    while (*msgPtr) {
        for (wordPtr = msgPtr; *wordPtr && *wordPtr != ' '; wordPtr++) ;
        wordLen = (uint8_t)(wordPtr - msgPtr);

        if (lineLen + wordLen + (lineLen > 0 ? 1 : 0) > MAX_WIDTH) {
            while (lineLen < MAX_WIDTH) lineBuffer[lineLen++] = ' ';
            lineBuffer[lineLen] = '\0';

            gotoxy(startCol, startRow++);
            cputc(CH_VLINE);
            cputs(lineBuffer);
            cputc(CH_VLINE);

            lineLen = 0;
        }

        if (lineLen > 0 && (lineLen + wordLen) < MAX_WIDTH) {
            lineBuffer[lineLen++] = ' ';
        }

        strncpy(lineBuffer + lineLen, msgPtr, wordLen);
        lineLen += wordLen;

        msgPtr = wordPtr;
        while (*msgPtr == ' ') msgPtr++;
    }

    if (lineLen > 0) {
        while (lineLen < MAX_WIDTH) lineBuffer[lineLen++] = ' ';
        lineBuffer[lineLen] = '\0';

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
