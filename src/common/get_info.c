/**
 * get_info.c
 *
 * Welcome screen and startup configuration.
 * Saved endpoint/name values are stored through fujinet-nio app-store.
 */

#include <conio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "appstore_settings.h"
#include "data.h"
#include "delay.h"
#include "get_line.h"
#include "sound.h"

/* Maximum lengths for user-entered strings */
#define ENDPOINT_LEN 60
#define NAME_LEN     8
#define FETCH_INTERVAL_LEN 6
#define FETCH_INTERVAL_DEFAULT 100U
#define FETCH_INTERVAL_MIN 10U
#define FETCH_INTERVAL_MAX 1000U

static char endpoint_input[ENDPOINT_LEN + 1];
static char fetch_interval_input[FETCH_INTERVAL_LEN];

static char *version = "nio-3.0.0";

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

void get_input(uint8_t x, uint8_t y, uint8_t len, char *s)
{
    memset(s, ' ', len - 1);
    s[len] = '\0';
    cputsxy(x, y, s);
    *s = '\0';
    gotoxy(x, y);
    cursor(1);
    get_line(s, len);
    clear_cursor();
}

void show_header(void)
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
    cputsxy(hxp, yps + 5, "            Version: 0.0.0      ");
    cputsxy(hxp + 21, yps + 5, version);
    chlinexy(hxp - 2, yps + 7, 36);
}

void show_server(char *s)
{
    cputsxy(txp, yps + 10, "Bounce Server URL:");
    cputsxy(txp, yps + 11, "> ");
    cputsxy(txp + 2, yps + 11, s);
}

void show_name(char *s)
{
    cputsxy(txp, yps + 13, "Your name (max 8):");
    cputsxy(txp, yps + 14, "> ");
    cputsxy(txp + 2, yps + 14, s);
}

static void show_fetch_interval(char *s)
{
    cputsxy(txp, yps + 16, "Fetch interval (ms):");
    cputsxy(txp, yps + 17, "> ");
    cputsxy(txp + 2, yps + 17, s);
}

static void cput_rev1(char *s)
{
    revers(1); cputc(s[0]);
    revers(0); cputs(&s[1]);
}

static void show_menu(void)
{
    cputsxy(txp + 2, 21, "Change ");
    cput_rev1("Server ");
    cputs(" ");
    cput_rev1("Name ");
    cputs(" ");
    cput_rev1("Fetch");
    revers(1);
    gotoxy(txp + 5, 22);
    cputs("Press a key to continue");
    revers(0);
    cursor(0);
}

static void show_store_error(void)
{
    char tmp[6];

    itoa((int)appstore_last_error(), tmp, 10);
    gotoxy(txp + 3, 18);
    cputs("Storage err ");
    cputs(tmp);
    cputs("   ");
}

static void clear_store_error(void)
{
    cputsxy(txp + 3, 18, "                  ");
}

static uint8_t parse_fetch_interval(const char *s)
{
    char *end;
    unsigned long value = strtoul(s, &end, 10);

    if (s[0] == '\0' || *end != '\0' || value < FETCH_INTERVAL_MIN ||
        value > FETCH_INTERVAL_MAX) {
        cputsxy(txp + 3, 18, "Rate 10-1000 ms  ");
        return 0;
    }
    fetch_interval_ms = (uint16_t)value;
    return 1;
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
                if (appstore_write_setting(endpoint_input, APPSTORE_KEY_ENDPOINT)) {
                    clear_store_error();
                } else {
                    show_store_error();
                }
                show_server(endpoint_input);
                break;

            case 'N':
            case 'n':
                get_input(txp + 2, yps + 14, NAME_LEN, name);
                if (appstore_write_setting(name, APPSTORE_KEY_NAME)) {
                    clear_store_error();
                } else {
                    show_store_error();
                }
                show_name(name);
                break;

            case 'F':
            case 'f':
                get_input(txp + 2, yps + 17, FETCH_INTERVAL_LEN,
                          fetch_interval_input);
                if (parse_fetch_interval(fetch_interval_input) &&
                    appstore_write_setting(fetch_interval_input,
                                           APPSTORE_KEY_FETCH_INTERVAL)) {
                    clear_store_error();
                } else if (appstore_last_error() != FN_OK) {
                    show_store_error();
                }
                itoa((int)fetch_interval_ms, fetch_interval_input, 10);
                show_fetch_interval(fetch_interval_input);
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
    fetch_interval_ms = FETCH_INTERVAL_DEFAULT;
    itoa((int)fetch_interval_ms, fetch_interval_input, 10);

    show_header();
    appstore_read_setting(endpoint_input, sizeof(endpoint_input), APPSTORE_KEY_ENDPOINT);
    appstore_read_setting(name, sizeof(name), APPSTORE_KEY_NAME);
    if (appstore_read_setting(fetch_interval_input, sizeof(fetch_interval_input),
                              APPSTORE_KEY_FETCH_INTERVAL)) {
        if (!parse_fetch_interval(fetch_interval_input)) {
            fetch_interval_ms = FETCH_INTERVAL_DEFAULT;
            itoa((int)fetch_interval_ms, fetch_interval_input, 10);
        }
    }
    show_server(endpoint_input);
    show_name(name);
    show_fetch_interval(fetch_interval_input);
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
