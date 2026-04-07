#ifndef BWC_LINUX_CONIO_H
#define BWC_LINUX_CONIO_H

#include <stdint.h>

#define CH_ULCORNER ((char)0x80)
#define CH_URCORNER ((char)0x81)
#define CH_LLCORNER ((char)0x82)
#define CH_LRCORNER ((char)0x83)
#define CH_HLINE    ((char)0x84)
#define CH_VLINE    ((char)0x85)
#define CH_CROSS    ((char)0x86)
#define CH_LTEE     ((char)0x87)
#define CH_RTEE     ((char)0x88)
#define CH_TTEE     ((char)0x89)
#define CH_BTEE     ((char)0x8a)

#define CH_LHALF    ((char)0x90)
#define CH_RHALF    ((char)0x91)
#define CH_BOTTOM   ((char)0x92)
#define CH_TOP      ((char)0x93)
#define CH_LLQUAD   ((char)0x94)
#define CH_LRQUAD   ((char)0x95)
#define CH_ULQUAD   ((char)0x96)
#define CH_URQUAD   ((char)0x97)
#define CH_URBLOCK  ((char)0x98)
#define CH_ULBLOCK  ((char)0x99)
#define CH_LRBLOCK  ((char)0x9a)
#define CH_LLBLOCK  ((char)0x9b)
#define CH_FULLBLOCK ((char)0x9c)
#define CH_CROSS_DIAG ((char)0x9d)
#define CH_CROSS_REV  ((char)0x9e)

#define CH_ENTER     13
#define CH_DEL       127
#define CH_CURS_LEFT 8

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

#endif /* BWC_LINUX_CONIO_H */
