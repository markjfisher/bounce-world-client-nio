#include <cc65.h>
#include <conio.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "app_errors.h"

#ifdef BWC_MSDOS_IOCTL_DIAG
#include "fn_msdos.h"
#endif

uint8_t err = 0;

#ifdef BWC_MSDOS_IOCTL_DIAG
static void put_hex8(uint8_t value)
{
    static const char hex[] = "0123456789ABCDEF";

    cputc(hex[(value >> 4) & 0x0F]);
    cputc(hex[value & 0x0F]);
}

static void put_uint16(uint16_t value)
{
    char tmp[7];

    itoa((int) value, tmp, 10);
    cputs(tmp);
}
#endif

void handle_err(char *reason)
{
    char tmp[6];
    if (err) {
        cursor(1);

        itoa((int) err, tmp, 10);

        gotoxy(0, 20);
        cputs("Error: ");
        cputs(reason);
        cputs(" : ");
        cputs(tmp);
        cputs("   ");

#ifdef BWC_MSDOS_IOCTL_DIAG
        if (fn_msdos_ioctl_last_detail() != FN_MSDOS_IOCTL_DETAIL_NONE ||
            fn_msdos_ioctl_last_error() != 0) {
            gotoxy(0, wherey() + 1);
            cputs("IOCTL d=");
            put_uint16(fn_msdos_ioctl_last_detail());
            cputs(" dev=");
            put_hex8(fn_msdos_ioctl_last_device());
            cputs(" cmd=");
            put_hex8(fn_msdos_ioctl_last_command());
            cputs(" nio=");
            put_uint16(fn_msdos_ioctl_last_nio_status());
            cputs(" len=");
            put_uint16(fn_msdos_ioctl_last_response_len());
            cputs(" dos=");
            put_uint16(fn_msdos_ioctl_last_error());
            cputs("   ");

            if (fn_msdos_ioctl_last_diag_error() != 0 ||
                fn_msdos_ioctl_last_diag_rx_len() != 0 ||
                fn_msdos_ioctl_last_diag_expected_len() != 0 ||
                fn_msdos_ioctl_last_diag_lsr() != 0) {
                gotoxy(0, wherey() + 1);
                cputs("DRV err=");
                put_uint16(fn_msdos_ioctl_last_diag_error());
                cputs(" st=");
                put_uint16(fn_msdos_ioctl_last_diag_status());
                cputs(" rx=");
                put_uint16(fn_msdos_ioctl_last_diag_rx_len());
                cputs(" exp=");
                put_uint16(fn_msdos_ioctl_last_diag_expected_len());
                cputs(" lsr=");
                put_hex8(fn_msdos_ioctl_last_diag_lsr());
                cputs("   ");
            }
        }
#endif

        gotoxy(0, wherey() + 1);

        if (doesclrscrafterexit()) {
            cgetc();
        }
        exit(1);
    }
}
