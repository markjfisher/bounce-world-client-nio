#ifndef BWC_APPSTORE_SETTINGS_H
#define BWC_APPSTORE_SETTINGS_H

#include <stdbool.h>
#include <stdint.h>

#define APPSTORE_KEY_ENDPOINT "endpoint"
#define APPSTORE_KEY_NAME     "name"

bool appstore_read_setting(char *buffer, uint8_t max_len, const char *key);
bool appstore_write_setting(const char *buffer, const char *key);
uint8_t appstore_last_error(void);

#endif /* BWC_APPSTORE_SETTINGS_H */
