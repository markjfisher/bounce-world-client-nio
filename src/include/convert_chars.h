#ifndef BWC_CONVERT_CHARS_H
#define BWC_CONVERT_CHARS_H

#include <stdint.h>

/*
 * Platform-specific character conversion from the server's "neutral" ASCII
 * encoding to the local platform's screen codes.
 *
 * Server encoding:
 *   'r'  -> box top-left corner
 *   ')'  -> box top-right corner
 *   'L'  -> box bottom-left corner
 *   '!'  -> box bottom-right corner
 *   'J'  -> right tee
 *   't'  -> left tee
 *   'T'  -> top tee
 *   '2'  -> bottom tee
 *   '|'  -> vertical line
 *   '-'  -> horizontal line
 *   '+'  -> cross
 *   'a'-'p' -> block/half-block graphics characters
 */
void convert_chars(uint8_t *data, uint8_t len);

#endif /* BWC_CONVERT_CHARS_H */
