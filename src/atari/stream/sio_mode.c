/* Polled stream traffic bypasses SIOV; do not alter U1MB or FujiNet HSIO. */
#include "sio_mode.h"
uint8_t bwc_standard_sio(void) { return 0; }
uint8_t bwc_restore_sio(void) { return 0; }
