#include "user_config.h"
#include "climate.h"
#include "settings.h"
#include "sensors.h"
#include "hal_timebase.h"

#define LONG_AGO_MS 3600000UL

static bool     forced[OUT_COUNT];
static uint32_t changed_ms[OUT_COUNT];

void climate_init(void)
{
    uint32_t now = millis();
    for (uint8_t i = 0; i < OUT_COUNT; i++) changed_ms[i] = now - LONG_AGO_MS;
}

bool climate_enabled(out_id_t id)
{
    switch (id) {
    case OUT_FAN:    return feature_on(FEAT_FAN);
    case OUT_HEATER: return feature_on(FEAT_HEATER);
    case OUT_LAMP:   return feature_on(FEAT_LAMP) && feature_on(FEAT_LIGHT);
    case OUT_PUMP:   return feature_on(FEAT_PUMP);      /* driven by irrigation.c, not by climate */
    default:         return false;
    }
}

/* Change an output, but not more often than min_s seconds (relay protection).
 * Safety switch-offs pass min_s = 0. */
static void apply(out_id_t id, bool want, uint32_t min_s)
{
    uint32_t now = millis();

    if (forced[id] || outputs_get(id) == want) return;
    if (now - changed_ms[id] < min_s * 1000UL) return;
    outputs_set(id, want);
    changed_ms[id] = now;
}

void climate_force(out_id_t id, bool on)
{
    forced[id] = true;
    outputs_set(id, on);
}

void climate_release(void)
{
    uint32_t now = millis();
    for (uint8_t i = 0; i < OUT_COUNT; i++) {
        if (forced[i]) {
            forced[i] = false;
            changed_ms[i] = now - LONG_AGO_MS;     /* auto logic may act at once */
        }
    }
}

static bool lamp_window(const sensors_t *s)
{
    const settings_t *c = &g_settings;
    if (c->lamp_start == c->lamp_end) return true;          /* 24 h */
    uint8_t h = s->time.hour;
    if (c->lamp_start < c->lamp_end) return h >= c->lamp_start && h < c->lamp_end;
    return h >= c->lamp_start || h < c->lamp_end;           /* crosses midnight */
}

void climate_update(void)
{
    const sensors_t  *s = sensors_get();
    const settings_t *c = &g_settings;
    const bool fan_en  = climate_enabled(OUT_FAN);
    const bool heat_en = climate_enabled(OUT_HEATER);
    const bool lamp_en = climate_enabled(OUT_LAMP);

    /* a disabled function always keeps its output off */
    if (!fan_en)  apply(OUT_FAN,    false, 0);
    if (!heat_en) apply(OUT_HEATER, false, 0);
    if (!lamp_en) apply(OUT_LAMP,   false, 0);

    if (!s->air_ok) {
        /* no temperature: heater is never allowed, fan optionally runs */
        apply(OUT_HEATER, false, 0);
        if (fan_en) apply(OUT_FAN, FAN_ON_SENSOR_FAIL != 0, 0);
    } else {
        int t = s->air_t;

        if (fan_en) {
            /* humidity venting only while it is not cold (never fight the heater) */
            bool humid = (c->hum_max < 100) && s->air_h >= c->hum_max &&
                         t > c->heat_on_c + CLIMATE_HYST_C;
            bool humid_clear = (c->hum_max >= 100) ||
                               s->air_h + CLIMATE_HUM_HYST <= c->hum_max;
            bool hot  = t >= c->fan_on_c;
            bool cool = t <= c->fan_on_c - CLIMATE_HYST_C;

            if (hot || humid)             apply(OUT_FAN, true,  CLIMATE_MIN_STATE_S);
            else if (cool && humid_clear) apply(OUT_FAN, false, CLIMATE_MIN_STATE_S);
        }

        if (heat_en) {
            if (outputs_get(OUT_FAN))                     apply(OUT_HEATER, false, 0);
            else if (t <= c->heat_on_c)                   apply(OUT_HEATER, true,  CLIMATE_MIN_STATE_S);
            else if (t >= c->heat_on_c + CLIMATE_HYST_C)  apply(OUT_HEATER, false, CLIMATE_MIN_STATE_S);
        }
    }

    /*
     * Lamp: allowed only inside its time window and only while the light sensor
     * works (no sensor / no clock = lamp off). ON when natural light is low,
     * OFF when it is bright again. The BH1750 must NOT be lit by the lamp itself,
     * otherwise it would switch itself off; the minimum state time also damps this.
     */
    if (lamp_en) {
        if (!s->rtc_ok || !s->light_ok || !lamp_window(s)) {
            apply(OUT_LAMP, false, 0);
        } else if (!outputs_get(OUT_LAMP)) {
            if (s->light_lux < c->light_low_lux)  apply(OUT_LAMP, true,  LAMP_MIN_STATE_S);
        } else {
            if (s->light_lux > c->light_high_lux) apply(OUT_LAMP, false, LAMP_MIN_STATE_S);
        }
    }
}
