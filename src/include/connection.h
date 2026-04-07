#ifndef BWC_CONNECTION_H
#define BWC_CONNECTION_H

#include <stdint.h>

void connect_service(void);
void disconnect_service(void);
void send_client_data(void);
void create_command(char *cmd);
void append_command(char *cmd);
void send_command(void);
int16_t read_response_wait(uint8_t *buf, int16_t len);
int16_t read_response_min(uint8_t *buf, int16_t min, int16_t max);
void request_client_data(void);

#endif /* BWC_CONNECTION_H */
