/* Keep display-list interrupts and U1MB SIO from interfering. */
#include <stdint.h>
#include "fujinet-nio.h"
#include "sio_mode.h"

static uint8_t saved_index;
static uint8_t restore_needed;

uint8_t bwc_standard_sio(void)
{
    uint8_t result;
    if (restore_needed || !fn_atari_u1mb_pbi()) return FN_OK;
    result = fn_fuji_get_hsio(&saved_index);
    if (result != FN_OK) return result;
    if (saved_index != FN_FUJI_HSIO_OFF) {
        result = fn_fuji_set_hsio(FN_FUJI_HSIO_OFF);
        if (result != FN_OK) return result;
        /* Even if clearing a table fails, an exit must restore the FujiNet. */
        restore_needed = 1;
    }
    result = fn_atari_u1mb_clear_devices(15);
    if (result != FN_OK) return result;
    return fn_atari_u1mb_clear_disks(0x7fff);
}

uint8_t bwc_restore_sio(void)
{
    uint8_t result;
    if (!restore_needed) return FN_OK;
    result = fn_fuji_set_hsio(saved_index);
    if (result != FN_OK) return result;
    result = fn_atari_u1mb_clear_devices(15);
    if (result != FN_OK) return result;
    result = fn_atari_u1mb_clear_disks(0x7fff);
    if (result == FN_OK) restore_needed = 0;
    return result;
}
