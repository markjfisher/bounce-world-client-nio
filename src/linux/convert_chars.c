#include <stdint.h>

#include "conio.h"
#include "convert_chars.h"

void convert_chars(uint8_t *data, uint8_t len)
{
    uint8_t i;

    for (i = 0; i < len; ++i) {
        switch (data[i]) {
            case 'r': data[i] = (uint8_t)CH_ULCORNER; break;
            case ')': data[i] = (uint8_t)CH_URCORNER; break;
            case 'L': data[i] = (uint8_t)CH_LLCORNER; break;
            case '!': data[i] = (uint8_t)CH_LRCORNER; break;
            case 'J': data[i] = (uint8_t)CH_RTEE;     break;
            case 't': data[i] = (uint8_t)CH_LTEE;     break;
            case 'T': data[i] = (uint8_t)CH_TTEE;     break;
            case '2': data[i] = (uint8_t)CH_BTEE;     break;
            case '|': data[i] = (uint8_t)CH_VLINE;    break;
            case '-': data[i] = (uint8_t)CH_HLINE;    break;
            case '+': data[i] = (uint8_t)CH_CROSS;    break;
            case 'a': data[i] = (uint8_t)CH_LHALF;    break;
            case 'b': data[i] = (uint8_t)CH_RHALF;    break;
            case 'c': data[i] = (uint8_t)CH_BOTTOM;   break;
            case 'd': data[i] = (uint8_t)CH_TOP;      break;
            case 'e': data[i] = (uint8_t)CH_LLQUAD;   break;
            case 'f': data[i] = (uint8_t)CH_LRQUAD;   break;
            case 'g': data[i] = (uint8_t)CH_ULQUAD;   break;
            case 'h': data[i] = (uint8_t)CH_URQUAD;   break;
            case 'i': data[i] = (uint8_t)CH_URBLOCK;  break;
            case 'j': data[i] = (uint8_t)CH_ULBLOCK;  break;
            case 'k': data[i] = (uint8_t)CH_LRBLOCK;  break;
            case 'l': data[i] = (uint8_t)CH_LLBLOCK;  break;
            case 'm': data[i] = (uint8_t)CH_FULLBLOCK; break;
            case 'n': data[i] = (uint8_t)CH_CROSS_DIAG; break;
            case 'p': data[i] = (uint8_t)CH_CROSS_REV;  break;

            default:
                break;
        }
    }
}
