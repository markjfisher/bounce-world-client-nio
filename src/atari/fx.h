#ifndef BWC_FX_H
#define BWC_FX_H

#include <stdbool.h>
#include <stdint.h>

void screen_flash(void);

extern bool    is_flashing_screen;
extern uint8_t current_flash_time;

#endif /* BWC_FX_H */
