#ifndef BWC_DISPLAY_H
#define BWC_DISPLAY_H

#include <stdint.h>

#ifdef __AMIGA__
#include "shape_decode.h"
#endif

void show_screen(void);
void show_info(void);
void init_screen(void);
void set_screen_colours(void);

#ifdef __AMIGA__
void amiga_show_screen_shapes(const ShapePos *shapes, uint8_t count);
#endif

#endif /* BWC_DISPLAY_H */
