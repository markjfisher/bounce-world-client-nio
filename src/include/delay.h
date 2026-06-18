#ifndef BWC_DELAY_H
#define BWC_DELAY_H

#include <stdint.h>

void wait_vsync(void);
#ifdef __CC65__
void __fastcall__ pause(uint8_t count);
#else
void pause(uint8_t count);
#endif

#endif /* BWC_DELAY_H */
