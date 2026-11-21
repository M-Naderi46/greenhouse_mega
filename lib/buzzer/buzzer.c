#include "user_config.h"
#include "buzzer.h"
#include "gpio.h"
#include "hal_timebase.h"

void buzzer_init(void)
{
    PIN_SET(!ALARM_ACTIVE_HIGH, ALARM_PIN);
    PIN_OUTPUT(ALARM_PIN);
}

void buzzer_task(bool alarm_on)
{
    static uint32_t t;
    uint32_t now = millis();

    if (alarm_on) {
        if (now - t >= 500) {
            t = now;
            PIN_TOGGLE(ALARM_PIN);
        }
    } else {
        PIN_SET(!ALARM_ACTIVE_HIGH, ALARM_PIN);
    }
}
