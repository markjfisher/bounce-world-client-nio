#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include "fujinet-nio.h"
#include "sio_mode.h"

static uint8_t pbi, index_value, fail_devices;
static unsigned get_count, sets, devices, disks;
uint8_t fn_atari_u1mb_pbi(void) { return pbi; }
uint8_t fn_fuji_get_hsio(uint8_t *p) { ++get_count; *p = index_value; return FN_OK; }
uint8_t fn_fuji_set_hsio(uint8_t v) { ++sets; index_value = v; return FN_OK; }
uint8_t fn_atari_u1mb_clear_devices(uint8_t mask) {
    assert(mask == 15); ++devices; return fail_devices ? FN_ERR_IO : FN_OK;
}
uint8_t fn_atari_u1mb_clear_disks(uint16_t mask) {
    assert(mask == 0x7fff); ++disks; return FN_OK;
}
int main(void) {
    assert(bwc_standard_sio() == FN_OK && !get_count && !sets && !devices);
    pbi = 1; index_value = 0;
    assert(bwc_standard_sio() == FN_OK);
    assert(index_value == FN_FUJI_HSIO_OFF && get_count == 1 && sets == 1 && devices == 1 && disks == 1);
    assert(bwc_standard_sio() == FN_OK && get_count == 1 && sets == 1);
    assert(bwc_restore_sio() == FN_OK);
    assert(index_value == 0 && sets == 2 && devices == 2 && disks == 2);
    assert(bwc_restore_sio() == FN_OK && sets == 2);
    index_value = FN_FUJI_HSIO_OFF;
    assert(bwc_standard_sio() == FN_OK && sets == 2 && devices == 3 && disks == 3);
    assert(bwc_restore_sio() == FN_OK && sets == 2);
    index_value = 9; fail_devices = 1;
    assert(bwc_standard_sio() == FN_ERR_IO && index_value == FN_FUJI_HSIO_OFF);
    fail_devices = 0;
    assert(bwc_restore_sio() == FN_OK && index_value == 9);
    puts("Atari SIO mode transitions: ok");
    return 0;
}
