#ifndef BWC_CONNECTION_H
#define BWC_CONNECTION_H

#include <stdint.h>

void append_command(char *cmd);
void connect_service(void);
void create_command(char *cmd);
void disconnect_service(void);
uint8_t request_client_data(void);
void send_client_data(void);
void send_command(void);

int16_t read_response_min(uint8_t *buf, int16_t min_payload, int16_t max_payload);
int16_t read_response_wait(uint8_t *payload_buf, int16_t payload_len);

#endif /* BWC_CONNECTION_H */
