#ifndef BWC_STREAM_NETWORK_H
#define BWC_STREAM_NETWORK_H
#include <stdint.h>
uint8_t bwc_stream_init(void);
uint8_t bwc_stream_open(const char *url);
uint8_t bwc_stream_close(void);
uint8_t bwc_stream_read(uint8_t *buf, uint16_t capacity, uint16_t *actual, uint8_t *flags);
uint8_t bwc_stream_write(const uint8_t *buf, uint16_t length, uint16_t *actual);
#endif
