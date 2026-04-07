#include <stdint.h>
#include "convert_chars.h"

/*
 * BBC Micro character conversion.
 *
 * BBC MODE 7 (Teletext) has a limited character set.  For other text modes
 * we use ASCII approximations for the box-drawing and block characters that
 * the server sends in its neutral encoding.
 */
void convert_chars(uint8_t *data, uint8_t len)
{
    uint8_t i;
    for (i = 0; i < len; i++) {
        switch (data[i]) {
            /* Box-drawing chars: use ASCII approximations */
            case 'r':  data[i] = '+'; break;   /* ┌ */
            case ')':  data[i] = '+'; break;   /* ┐ */
            case 'L':  data[i] = '+'; break;   /* └ */
            case '!':  data[i] = '+'; break;   /* ┘ */
            case 'J':  data[i] = '+'; break;   /* ┤ */
            case 't':  data[i] = '+'; break;   /* ├ */
            case 'T':  data[i] = '+'; break;   /* ┬ */
            case '2':  data[i] = '+'; break;   /* ┴ */
            case '|':  /* keep */     break;   /* │ */
            case '-':  /* keep */     break;   /* ─ */
            case '+':  /* keep */     break;   /* ┼ */

            /* Block graphics: use '#' for filled, '.' for quarter-blocks */
            case 'a':
            case 'b':
            case 'i':
            case 'j':
            case 'k':
            case 'l':
            case 'm':  data[i] = '#'; break;

            case 'c':
            case 'd':  data[i] = '-'; break;

            case 'e':
            case 'f':
            case 'g':
            case 'h':  data[i] = '.'; break;

            default: break;
        }
    }
}
