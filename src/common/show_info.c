#include <conio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "data.h"
#include "debug.h"
#include "world.h"
#include "screen.h"

/* Print a uint8_t right-aligned in 2 chars */
static void printu8j2(uint8_t v)
{
    if (v < 10) {
        cputc((char)(v + '0'));
        cputc(' ');
    } else {
        cputc((char)(v / 10 + '0'));
        cputc((char)(v % 10 + '0'));
    }
}

/* Print a uint16 decimal */
static void printu16(uint16_t v)
{
    char tmp[6];
    utoa(v, tmp, 10);
    cputs(tmp);
}

static void print_reverse(char *s)
{
    revers(1); cputs(s); revers(0);
}

void show_info(void)
{
    uint8_t i;

    cputsxy(0, SCREEN_HEIGHT - 2, name);
    for (i = 0; i < name_pad; i++) {
        cputc(' ');
    }

    revers(1); cputs("C:"); revers(0); printu8j2(num_clients);
    revers(1); cputs("1:"); revers(0); printu8j2(body_1);
    revers(1); cputs("2:"); revers(0); printu8j2(body_2);
    revers(1); cputs("3:"); revers(0); printu8j2(body_3);
    revers(1); cputs("4:"); revers(0); printu8j2(body_4);
    revers(1); cputs("5:"); revers(0); printu8j2(body_5);

    if (world_height > 99) {
        cputc(' ');
    }
    if (world_is_frozen) {
        revers(1);
    }
    printu16(world_width);
    cputc('x');
    printu16(world_height);
    if (world_is_frozen) {
        revers(0);
    }

    gotoxy(0, SCREEN_HEIGHT - 1);
    print_reverse("F"); cputs("rz ");
    print_reverse("R"); cputs("st ");
    print_reverse("+"); cputc('/'); print_reverse("-"); cputc(' ');
    print_reverse("1"); cputc('-'); print_reverse("5"); cputs("Add ");
    print_reverse("W"); cputs("ho ");
    print_reverse("I"); cputs("nf ");
    print_reverse("Q"); cputs("uit ");

#ifdef __AMIGA__
    print_reverse("V"); cputs("ec ");
    print_reverse("O"); cputs("vl ");
#endif

#ifdef __BBC__
    print_reverse("C"); cputs("ol ");
#endif

#ifdef __ATARI__
    print_reverse("D"); cputs("rk ");
    cputc('f'); print_reverse("L"); cputs("sh");
#endif
}
