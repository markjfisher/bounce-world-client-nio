#ifndef BWC_MSDOS_CONIO_H
#define BWC_MSDOS_CONIO_H

#include <stdint.h>

#define CH_ULCORNER ((char)0xda)
#define CH_URCORNER ((char)0xbf)
#define CH_LLCORNER ((char)0xc0)
#define CH_LRCORNER ((char)0xd9)
#define CH_HLINE    ((char)0xc4)
#define CH_VLINE    ((char)0xb3)
#define CH_CROSS    ((char)0xc5)
#define CH_LTEE     ((char)0xc3)
#define CH_RTEE     ((char)0xb4)
#define CH_TTEE     ((char)0xc2)
#define CH_BTEE     ((char)0xc1)

#define CH_LHALF    ((char)0xdd)
#define CH_RHALF    ((char)0xde)
#define CH_BOTTOM   ((char)0xdc)
#define CH_TOP      ((char)0xdf)
#define CH_LLQUAD   ((char)0xb0)
#define CH_LRQUAD   ((char)0xb1)
#define CH_ULQUAD   ((char)0xb2)
#define CH_URQUAD   ((char)0xb0)
#define CH_URBLOCK  ((char)0xdb)
#define CH_ULBLOCK  ((char)0xdb)
#define CH_LRBLOCK  ((char)0xdb)
#define CH_LLBLOCK  ((char)0xdb)
#define CH_FULLBLOCK ((char)0xdb)
#define CH_CROSS_DIAG ((char)0x2f)
#define CH_CROSS_REV  ((char)0x5c)

#define CH_ENTER     13
#define CH_DEL       8
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

char *itoa(int value, char *str, int base);
char *utoa(unsigned int value, char *str, int base);

#endif /* BWC_MSDOS_CONIO_H */
