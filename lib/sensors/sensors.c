#include "user_config.h"
#include <stdio.h>
#include "sensors.h"
#include "settings.h"
#include "analog.h"
#include "soil_sensor.h"
#include "light_sensor.h"
#include "air_sensor.h"
#include "tank.h"

static sensors_t s;
static uint8_t air_fail;
static uint8_t air_type = 0xFF;      /* sensor type the current readings belong to */

void sensors_init(void)
{
    air_init();
    tank_init();
    light_init();            /* starts BH1750 early so the first reading is valid */
}

static void combine_soil(void)
{
    uint8_t n = 0;
    uint16_t sum = 0;
    uint8_t lowest = 100;

    for (uint8_t i = 0; i < s.soil_count; i++) {
        if (!s.soil_valid[i]) continue;
        n++;
        sum += s.soil_p[i];
        if (s.soil_p[i] < lowest) lowest = s.soil_p[i];
    }

    s.soil_ok = (n > 0);
    if (!n) {
        s.soil_pct = 0;
    } else {
#if SOIL_COMBINE_MODE == 1
        s.soil_pct = lowest;                    /* driest probe wins */
#else
        s.soil_pct = (uint8_t)(sum / n);        /* average of working probes */
#endif
    }
}

void sensors_update(void)
{
    s.rtc_ok = rtc_read(&s.time);

    s.soil_count = feature_on(FEAT_SOIL2) ? 2 : 1;
    soil_read_all(s.soil_raw, s.soil_count);
    for (uint8_t i = 0; i < 2; i++) {
        if (i < s.soil_count) {
            s.soil_valid[i] = soil_raw_valid(s.soil_raw[i]);
            s.soil_p[i] = analog_to_percent(s.soil_raw[i],
                                            g_settings.soil_dry_raw[i],
                                            g_settings.soil_wet_raw[i]);
        } else {
            s.soil_valid[i] = false;
            s.soil_p[i] = 0;
        }
    }
    combine_soil();

    if (feature_on(FEAT_LIGHT)) {
        s.light_ok = light_read_lux(&s.light_lux);
    } else {
        s.light_ok = false;
        s.light_lux = 0;
    }

    tank_update();
    s.tank_empty = feature_on(FEAT_TANK) && tank_empty();
}

void sensors_update_air(void)
{
    int16_t t10;
    uint8_t h;

    if (g_settings.air_sensor != air_type) {    /* type changed in the menu: restart */
        air_type = g_settings.air_sensor;
        air_init();
        s.air_ok = false;                       /* old readings belong to another sensor */
        air_fail = 5;
    }

    if (air_read(&t10, &h)) {
        s.air_t10 = t10;
        s.air_t = (int8_t)((t10 >= 0) ? (t10 + 5) / 10 : -((-t10 + 5) / 10));   /* nearest whole degree */
        s.air_h = h;
        s.air_ok = true;
        air_fail = 0;
    } else if (++air_fail >= 5) {      /* 5 failures in a row -> report error */
        s.air_ok = false;
        air_fail = 5;
    }
}

const sensors_t *sensors_get(void)
{
    return &s;
}

void sensors_fmt_temp(char *buf, unsigned size, int16_t t10)
{
    unsigned a = (unsigned)(t10 < 0 ? -t10 : t10);
    snprintf(buf, size, "%s%u.%u", t10 < 0 ? "-" : "", a / 10, a % 10);
}
