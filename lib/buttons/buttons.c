#include "user_config.h"
#include <stdbool.h>
#include "buttons.h"
#include "gpio.h"
#include "hal_timebase.h"

#define LONG_PRESS_MS 1000UL

static uint8_t  last;
static uint8_t  repeats;
static uint32_t t_scan, t_hold, t_mode;
static bool     mode_long;

void buttons_init(void)
{
    PIN_INPUT(BTN_MODE_PIN);
    PIN_INPUT(BTN_UP_PIN);
    PIN_INPUT(BTN_DOWN_PIN);
#if BTN_ACTIVE_LOW
    PIN_HIGH(BTN_MODE_PIN);               /* internal pull-ups */
    PIN_HIGH(BTN_UP_PIN);
    PIN_HIGH(BTN_DOWN_PIN);
#else
    PIN_LOW(BTN_MODE_PIN);                /* AVR has no pull-down: use external 10k to GND */
    PIN_LOW(BTN_UP_PIN);
    PIN_LOW(BTN_DOWN_PIN);
#endif
}

static uint8_t read_keys(void)
{
    uint8_t k = 0;
    if (PIN_READ(BTN_MODE_PIN) != (bool)BTN_ACTIVE_LOW) k |= KEY_MODE;
    if (PIN_READ(BTN_UP_PIN)   != (bool)BTN_ACTIVE_LOW) k |= KEY_UP;
    if (PIN_READ(BTN_DOWN_PIN) != (bool)BTN_ACTIVE_LOW) k |= KEY_DOWN;
    return k;
}

uint8_t buttons_poll(void)
{
    uint32_t now = millis();
    if (now - t_scan < 25) return 0;          /* 25 ms scan = debounce */
    t_scan = now;

    uint8_t cur = read_keys();
    uint8_t press = cur & (uint8_t)~last;
    uint8_t ev = press & (KEY_UP | KEY_DOWN);

    /* MODE: short press on release, long press while held */
    if (press & KEY_MODE) { t_mode = now; mode_long = false; }
    if ((cur & KEY_MODE) && !mode_long && (now - t_mode) >= LONG_PRESS_MS) {
        ev |= KEY_MODE_LONG;
        mode_long = true;
    }
    if ((last & KEY_MODE) && !(cur & KEY_MODE) && !mode_long) ev |= KEY_MODE;

    /* UP / DOWN auto-repeat */
    if (cur != last) {
        t_hold = now;
        repeats = 0;
    } else if ((cur & (KEY_UP | KEY_DOWN)) && (now - t_hold) > 500) {
        ev |= cur & (KEY_UP | KEY_DOWN);
        t_hold = now - 350;                   /* then every 150 ms */
        if (repeats < 255) repeats++;
    }
    last = cur;
    return ev;
}

uint8_t buttons_repeat_count(void)
{
    return repeats;
}
