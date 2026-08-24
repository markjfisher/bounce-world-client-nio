#ifndef BWC_AMIGA_CONIO_H
#define BWC_AMIGA_CONIO_H

#include <stdint.h>

#define CH_ULCORNER  ((char)'+')
#define CH_URCORNER  ((char)'+')
#define CH_LLCORNER  ((char)'+')
#define CH_LRCORNER  ((char)'+')
#define CH_HLINE     ((char)'-')
#define CH_VLINE     ((char)'|')
#define CH_CROSS     ((char)'+')
#define CH_LTEE      ((char)'+')
#define CH_RTEE      ((char)'+')
#define CH_TTEE      ((char)'+')
#define CH_BTEE      ((char)'+')

#define CH_LHALF     ((char)'-')
#define CH_RHALF     ((char)'-')
#define CH_BOTTOM    ((char)'-')
#define CH_TOP       ((char)'-')
#define CH_LLQUAD    ((char)'+')
#define CH_LRQUAD    ((char)'+')
#define CH_ULQUAD    ((char)'+')
#define CH_URQUAD    ((char)'+')
#define CH_URBLOCK   ((char)'#')
#define CH_ULBLOCK   ((char)'#')
#define CH_LRBLOCK   ((char)'#')
#define CH_LLBLOCK   ((char)'#')
#define CH_FULLBLOCK ((char)'#')
#define CH_CROSS_DIAG ((char)'#')
#define CH_CROSS_REV  ((char)'#')

#define CH_ENTER     13
#define CH_DEL       127
#define CH_CURS_LEFT 8
#define CH_STOP      3

void clrscr(void);
void gotoxy(uint8_t x, uint8_t y);
void gotox(uint8_t x);
uint8_t wherex(void);
uint8_t wherey(void);
void cputc(char c);
void cputs(const char *s);
void cputcxy(uint8_t x, uint8_t y, char c);
void cputsxy(uint8_t x, uint8_t y, const char *s);
void revers(uint8_t on);
void cursor(uint8_t on);
void chlinexy(uint8_t x, uint8_t y, uint8_t len);
uint8_t kbhit(void);
char cgetc(void);

/* Amiga-shim internal hooks shared by the other platform shim files */
void *amiga_conio_draw_rp(void);
void amiga_conio_clear(void);
void amiga_conio_present(void);
void amiga_conio_swap(void);
uint16_t amiga_conio_height(void);

static inline char *itoa(int value, char *str, int base)
{
    char digits[] = "0123456789abcdefghijklmnopqrstuvwxyz";
    char tmp[34];
    unsigned int magnitude;
    int negative = 0;
    int i = 0;
    int j = 0;

    if (base < 2 || base > 36) {
        str[0] = '\0';
        return str;
    }

    if (value < 0 && base == 10) {
        negative = 1;
        magnitude = (unsigned int)(-value);
    } else {
        magnitude = (unsigned int)value;
    }

    do {
        tmp[i++] = digits[magnitude % (unsigned int)base];
        magnitude /= (unsigned int)base;
    } while (magnitude != 0U);

    if (negative) {
        tmp[i++] = '-';
    }

    while (i > 0) {
        str[j++] = tmp[--i];
    }
    str[j] = '\0';
    return str;
}

static inline char *utoa(unsigned int value, char *str, int base)
{
    char digits[] = "0123456789abcdefghijklmnopqrstuvwxyz";
    char tmp[34];
    int i = 0;
    int j = 0;

    if (base < 2 || base > 36) {
        str[0] = '\0';
        return str;
    }

    do {
        tmp[i++] = digits[value % (unsigned int)base];
        value /= (unsigned int)base;
    } while (value != 0U);

    while (i > 0) {
        str[j++] = tmp[--i];
    }
    str[j] = '\0';
    return str;
}

#endif /* BWC_AMIGA_CONIO_H */
