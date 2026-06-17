#ifndef BWC_DELAY_H
#define BWC_DELAY_H

#include <stdint.h>

void wait_vsync(void);
void __fastcall__ pause(uint8_t count);

#endif /* BWC_DELAY_H */
