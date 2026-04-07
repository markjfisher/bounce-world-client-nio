/**
 * get_info.c
 *
 * Welcome screen and startup configuration.
 * Appkey/persistent storage is not available in fujinet-nio-lib v1,
 * so the user must enter the server URL and their name on every startup.
 */

#include <conio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "data.h"
#include "delay.h"
#include "get_line.h"
#include "sound.h"
#include "press_key.h"

/* Maximum lengths for user-entered strings */
#define ENDPOINT_LEN 60
#define NAME_LEN     8

static char endpoint_input[ENDPOINT_LEN + 1];

static char *version = "2.0.3-nio";

static char hxp = 4;
static char txp = 3;
static char yps = 3;

/* -----------------------------------------------------------------------
 * Display helpers
 * --------------------------------------------------------------------- */

static void clear_cursor(void)
{
    cursor(0);
    cputcxy(0, 0, ' ');
}

static void get_input(uint8_t x, uint8_t y, uint8_t len, char *s)
{
    memset(s, ' ', len);
    cputsxy(x, y, s);
    *s = '\0';
    gotoxy(x, y);
    cursor(1);
    get_line(s, len);
    clear_cursor();
}

static void show_header(void)
{
    clrscr();
    init_sound();
    chlinexy(hxp - 2, yps - 1, 36);
    revers(1);
    cputsxy(hxp, yps + 1, "                                ");
    cputsxy(hxp, yps + 2, " Welcome to Bouncy World Client ");
    cputsxy(hxp, yps + 3, "        By Mark Fisher          ");
    cputsxy(hxp, yps + 4, "                                ");
    revers(0);
    cputsxy(hxp, yps + 5, "                Version: 0.0.0  ");
    cputsxy(hxp + 25, yps + 5, version);
    chlinexy(hxp - 2, yps + 7, 36);
}

static void show_server(char *s)
{
    cputsxy(txp, yps + 10, "Bounce Server URL:");
    cputsxy(txp, yps + 11, "> ");
    cputsxy(txp + 2, yps + 11, s);
}

static void show_name(char *s)
{
    cputsxy(txp, yps + 13, "Your name (max 8):");
    cputsxy(txp, yps + 14, "> ");
    cputsxy(txp + 2, yps + 14, s);
}

static void cput_rev1(char *s)
{
    revers(1); cputc(s[0]);
    revers(0); cputs(&s[1]);
}

static void show_menu(void)
{
    chlinexy(txp + 3, 20, 28);
    cputsxy(txp + 4, 21, "Change ");
    cput_rev1("Server ");
    cputs("Change ");
    cput_rev1("Name");
    revers(1);
    gotoxy(txp + 5, 22);
    cputs("Press a key to continue");
    revers(0);
    chlinexy(txp + 3, 23, 28);
}

/* -----------------------------------------------------------------------
 * Input loop: S = change server, N = change name, other = continue
 * --------------------------------------------------------------------- */

static void get_info_changes(void)
{
    char c;

    while (1) {
        c = 0;
        while (kbhit() == 0) ;
        c = cgetc();

        switch (c) {
            case 'S':
            case 's':
                get_input(txp + 2, yps + 11, ENDPOINT_LEN, endpoint_input);
                show_server(endpoint_input);
                break;

            case 'N':
            case 'n':
                get_input(txp + 2, yps + 14, NAME_LEN, name);
                show_name(name);
                break;

            default:
                /* Only proceed once both fields are filled */
                if (strlen(endpoint_input) > 0 && strlen(name) > 0) {
                    return;
                }
                break;
        }
    }
}

/* -----------------------------------------------------------------------
 * Public entry point
 * --------------------------------------------------------------------- */

void get_info(void)
{
    memset(endpoint_input, 0, sizeof(endpoint_input));
    memset(name, 0, sizeof(name));

    show_header();
    show_server(endpoint_input);
    show_name(name);
    show_menu();

    get_info_changes();
    clear_cursor();

    /* Build server_url: prepend "tcp://" scheme if not already present */
    memset(server_url, 0, sizeof(server_url));
    if (strncmp(endpoint_input, "tcp://", 6) != 0 &&
        strncmp(endpoint_input, "tls://", 6) != 0) {
        strcpy(server_url, "tcp://");
    }
    strcat(server_url, endpoint_input);

    /* Pre-calculate name padding width for the info bar */
    name_pad = 9 - (uint8_t)strlen(name);
}
