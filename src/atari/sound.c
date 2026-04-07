#include <atari.h>
#include <conio.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "data.h"
#include "delay.h"
#include "debug.h"
#include "sound.h"

bool    is_playing_collision  = false;
uint8_t current_volume_index  = 0;

void init_sound(void)
{
    OS.soundr = 0;
}

void stop_sound(void)
{
    _sound(0, 0, 0, 0);
    _sound(1, 0, 0, 0);
    _sound(2, 0, 0, 0);
}

void sound_collision(void)
{
    is_playing_collision = true;
    current_volume_index = 0;
    _sound(0, 80, 8, 15);
}
