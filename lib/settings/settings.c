#include "user_config.h"
#include <avr/eeprom.h>
#include "settings.h"

settings_t g_settings;
static settings_t ee_settings EEMEM;

static uint8_t crc8(const uint8_t *d, uint8_t n)
{
    uint8_t crc = 0xFF;
    while (n--) {
        crc ^= *d++;
        for (uint8_t i = 0; i < 8; i++)
            crc = (crc & 0x80) ? (uint8_t)((crc << 1) ^ 0x07) : (uint8_t)(crc << 1);
    }
    return crc;
}

static uint8_t calc_crc(const settings_t *s)
{
    return crc8((const uint8_t *)s, (uint8_t)(sizeof(settings_t) - 1));
}

void settings_defaults(settings_t *s)
{
    s->version          = SETTINGS_VERSION;
    s->features         = (DEFAULT_IRRIG_ENABLED  ? FEAT_IRRIG  : 0) |
                          (DEFAULT_SOIL2_ENABLED  ? FEAT_SOIL2  : 0) |
                          (DEFAULT_LIGHT_ENABLED  ? FEAT_LIGHT  : 0) |
                          (DEFAULT_TANK_ENABLED   ? FEAT_TANK   : 0) |
                          (DEFAULT_PUMP_ENABLED   ? FEAT_PUMP   : 0) |
                          (DEFAULT_FAN_ENABLED    ? FEAT_FAN    : 0) |
                          (DEFAULT_HEATER_ENABLED ? FEAT_HEATER : 0) |
                          (DEFAULT_LAMP_ENABLED   ? FEAT_LAMP   : 0) |
                          (DEFAULT_LOG_ENABLED    ? FEAT_LOG    : 0);
    s->air_sensor       = DEFAULT_AIR_SENSOR;
    s->soil_low         = DEFAULT_SOIL_LOW;
    s->soil_high        = DEFAULT_SOIL_HIGH;
    s->win_start        = DEFAULT_WIN_START;
    s->win_end          = DEFAULT_WIN_END;
    s->max_liters       = DEFAULT_MAX_LITERS;
    s->max_run_min      = DEFAULT_MAX_RUN_MIN;
    s->cooldown_min     = DEFAULT_COOLDOWN_MIN;
    s->min_temp_c       = DEFAULT_MIN_TEMP_C;
    s->pulses_per_liter = DEFAULT_PULSES_PER_LITER;
    s->soil_dry_raw[0]  = DEFAULT_SOIL_DRY_RAW;
    s->soil_dry_raw[1]  = DEFAULT_SOIL_DRY_RAW;
    s->soil_wet_raw[0]  = DEFAULT_SOIL_WET_RAW;
    s->soil_wet_raw[1]  = DEFAULT_SOIL_WET_RAW;
    s->fan_on_c         = DEFAULT_FAN_ON_C;
    s->heat_on_c        = DEFAULT_HEAT_ON_C;
    s->hum_max          = DEFAULT_HUM_MAX;
    s->lamp_start       = DEFAULT_LAMP_START;
    s->lamp_end         = DEFAULT_LAMP_END;
    s->light_low_lux    = DEFAULT_LIGHT_LOW_LUX;
    s->light_high_lux   = DEFAULT_LIGHT_HIGH_LUX;
    s->crc              = calc_crc(s);
}

static bool cal_ok(uint16_t a, uint16_t b)
{
    int32_t diff = (int32_t)a - (int32_t)b;
    if (diff < 0) diff = -diff;
    return a <= ANALOG_MAX && b <= ANALOG_MAX && diff >= CAL_MIN_SPAN;
}

bool settings_check(const settings_t *s)
{
    return
        s->air_sensor < 3 &&                       /* AIR_COUNT */
        /* irrigation */
        s->soil_low < s->soil_high && s->soil_high <= 99 &&
        s->win_start < 24 && s->win_end < 24 &&
        s->max_liters >= 1 && s->max_liters <= 99 &&
        s->max_run_min >= 1 && s->max_run_min <= 60 &&
        s->cooldown_min <= 240 &&
        s->min_temp_c >= -10 && s->min_temp_c <= 15 &&
        s->pulses_per_liter >= 10 && s->pulses_per_liter <= 9999 &&
        cal_ok(s->soil_dry_raw[0], s->soil_wet_raw[0]) &&
        cal_ok(s->soil_dry_raw[1], s->soil_wet_raw[1]) &&
        /* climate: fan and heater thresholds must not overlap */
        s->heat_on_c >= -5 && s->heat_on_c <= 30 &&
        s->fan_on_c >= 15 && s->fan_on_c <= 50 &&
        (int16_t)s->fan_on_c - (int16_t)s->heat_on_c >= 6 &&
        s->hum_max >= 40 && s->hum_max <= 100 &&
        /* lamp */
        s->lamp_start < 24 && s->lamp_end < 24 &&
        s->light_low_lux < s->light_high_lux && s->light_high_lux <= 60000;
}

bool settings_store(const settings_t *s)
{
    if (!settings_check(s)) return false;

    settings_t t = *s;
    t.version = SETTINGS_VERSION;
    t.crc = calc_crc(&t);

    g_settings = t;
    eeprom_update_block(&t, &ee_settings, sizeof t);
    return true;
}

void settings_load(void)
{
    settings_t t;
    eeprom_read_block(&t, &ee_settings, sizeof t);

    if (t.version == SETTINGS_VERSION && t.crc == calc_crc(&t) && settings_check(&t)) {
        g_settings = t;
    } else {
        settings_defaults(&t);          /* first start, old version or corrupted EEPROM */
        settings_store(&t);
    }
}
