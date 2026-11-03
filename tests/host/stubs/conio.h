#ifndef TEST_CONIO_H
#define TEST_CONIO_H
void gotoxy(unsigned char x, unsigned char y);
void cputc(char c);
void cputs(const char *s);
char *itoa(int value, char *buf, int base);
#endif
