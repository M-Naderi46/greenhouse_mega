#include "user_config.h"
#include <util/delay.h>
#include "soil_sensor.h"
#include "analog.h"
#include "gpio.h"

void soil_init(void)
{
#if SOIL_POWER_SWITCHED
    PIN_SET(0, SOIL_POWER_PIN);
    PIN_OUTPUT(SOIL_POWER_PIN);
#endif
}

void soil_read_all(uint16_t raw[2], uint8_t count)
{
#if SOIL_POWER_SWITCHED
    PIN_SET(1, SOIL_POWER_PIN);
    _delay_ms(30);
#endif
    raw[0] = analog_read(SOIL1_PIN);
    raw[1] = (count > 1) ? analog_read(SOIL2_PIN) : 0;
#if SOIL_POWER_SWITCHED
    PIN_SET(0, SOIL_POWER_PIN);
#endif
}

bool soil_raw_valid(uint16_t raw)
{
    return raw > SOIL_RAW_MIN_VALID && raw < SOIL_RAW_MAX_VALID;
}
