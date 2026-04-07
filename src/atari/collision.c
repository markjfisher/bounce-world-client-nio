#include "collision.h"
#include "data.h"
#include "fx.h"
#include "sound.h"

void collision_fx(void)
{
    if (flash_on_collision) {
        screen_flash();
    }
}
