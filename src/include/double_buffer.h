#ifndef BWC_DOUBLE_BUFFER_H
#define BWC_DOUBLE_BUFFER_H

#include <stdint.h>

#ifdef __ATARI__
extern uint8_t *dlist_scr_ptr;
extern char *screen_mem_orig;
#endif

extern void swap_buffer(void);
extern uint8_t is_alt_screen;
extern void show_other_screen(void);

#endif /* BWC_DOUBLE_BUFFER_H */
