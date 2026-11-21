#include "user_config.h"
#include "valve.h"
#include "gpio.h"

static bool is_open;

void valve_set(bool open)
{
    PIN_SET(VALVE_ACTIVE_HIGH ? open : !open, VALVE_PIN);
    is_open = open;
}

void valve_init(void)
{
    valve_set(false);                 /* set level first, then make it an output */
    PIN_OUTPUT(VALVE_PIN);
}

bool valve_is_open(void)
{
    return is_open;
}
