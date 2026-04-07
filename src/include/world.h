#ifndef BWC_WORLD_H
#define BWC_WORLD_H

#include <stdint.h>

extern uint16_t world_width;
extern uint16_t world_height;
extern uint8_t  num_clients;
extern uint8_t  world_is_frozen;
extern uint8_t  world_is_wrapped;
extern uint16_t body_count;
extern uint8_t  body_1;
extern uint8_t  body_2;
extern uint8_t  body_3;
extern uint8_t  body_4;
extern uint8_t  body_5;

void get_world_state(void);
void get_world_clients(void);
void get_world_cmd(void);
void get_broadcast(void);
int16_t fetch_client_state(void);
void create_client_data_command(void);

#endif /* BWC_WORLD_H */
