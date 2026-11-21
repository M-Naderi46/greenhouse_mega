#include "user_config.h"
#include <stdint.h>
#include "tank.h"
#include "gpio.h"

static bool    empty;
static uint8_t counter;

void tank_init(void)
{
    PIN_INPUT(TANK_PIN);
    PIN_HIGH(TANK_PIN);                       /* internal pull-up */
    empty = false;
    counter = 0;
}

void tank_update(void)
{
    bool pin_high = PIN_READ(TANK_PIN);
    bool raw_empty = TANK_EMPTY_WHEN_OPEN ? pin_high : !pin_high;

    if (raw_empty == empty) {
        counter = 0;
    } else if (++counter >= TANK_DEBOUNCE_S) {      /* state must be stable */
        empty = raw_empty;
        counter = 0;
    }
}

bool tank_empty(void)
{
    return empty;
}
