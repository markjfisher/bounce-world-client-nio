#ifndef BWC_ATARI_TITLE_H
#define BWC_ATARI_TITLE_H
#include <stdint.h>
void title_draw(void);
void title_server(const char *value);
void title_name(const char *value);
void title_enable(void);
void title_disable(void);
void title_animate(void);
void title_edit(char *buffer, uint8_t capacity, uint8_t row);
void title_fx_start(void);
void title_fx_stop(void);
void title_fx_pause(void);
void title_fx_resume(void);
#endif
