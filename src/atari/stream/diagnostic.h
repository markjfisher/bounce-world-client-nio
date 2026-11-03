#ifndef BWC_STREAM_DIAGNOSTIC_H
#define BWC_STREAM_DIAGNOSTIC_H
#ifdef BWC_STREAM_DIAG
#include <stdint.h>
void bwc_diag_start(void);
void bwc_diag_stop(void);
void bwc_diag_refresh(void);
void bwc_diag_begin(void);
void bwc_diag_end(uint8_t changed);
void bwc_diag_io_begin(uint8_t phase);
void bwc_diag_io_end(uint8_t status);
#else
#define bwc_diag_start() ((void)0)
#define bwc_diag_begin() ((void)0)
#define bwc_diag_end(changed) ((void)0)
#define bwc_diag_io_begin(phase) ((void)0)
#define bwc_diag_io_end(status) ((void)0)
#endif
#endif
