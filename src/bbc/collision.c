#include "collision.h"
#include "data.h"
#include "sound.h"

/* No hardware screen-flash available on BBC; could extend later */
void collision_fx(void)
{
    if (flash_on_collision) {
        /* TODO: implement flash via background colour change */
    }
}
