#ifndef SETTINGS_H
#define SETTINGS_H

#include <stdbool.h>
#include <stdint.h>

#define SETTINGS_VERSION 6

/* Feature switches (settings_t.features) - editable in menu > FEATURES */
#define FEAT_IRRIG   0x0001   /* automatic watering                 */
#define FEAT_SOIL2   0x0002   /* second soil probe                  */
#define FEAT_LIGHT   0x0004   /* BH1750 light sensor                */
#define FEAT_TANK    0x0008   /* tank float switch                  */
#define FEAT_FAN     0x0010   /* fan control                        */
#define FEAT_HEATER  0x0020   /* heater control                     */
#define FEAT_LAMP    0x0040   /* grow-lamp control (needs FEAT_LIGHT) */
#define FEAT_LOG     0x0080   /* UART data log                      */
#define FEAT_PUMP    0x0100   /* water pump (tank mode)             */

#define FEAT_CLIMATE (FEAT_FAN | FEAT_HEATER | FEAT_LAMP)   /* greenhouse-only functions */

typedef struct __attribute__((packed)) {
    uint8_t  version;
    uint16_t features;           /* FEAT_* bit mask                        */
    uint8_t  air_sensor;         /* 0 = DHT11, 1 = DHT12, 2 = AHT20 (see air_sensor.h) */
    /* irrigation */
    uint8_t  soil_low;           /* % : start watering below               */
    uint8_t  soil_high;          /* % : stop watering above                */
    uint8_t  win_start;          /* allowed window, start hour             */
    uint8_t  win_end;            /* allowed window, end hour (== start -> 24 h) */
    uint8_t  max_liters;         /* litres per cycle                       */
    uint8_t  max_run_min;        /* minutes per cycle                      */
    uint8_t  cooldown_min;       /* pause between cycles                   */
    int8_t   min_temp_c;         /* no watering at/below this air temp     */
    uint16_t pulses_per_liter;   /* flow meter calibration                 */
    uint16_t soil_dry_raw[2];    /* ADC value in air   (probe 1, probe 2)  */
    uint16_t soil_wet_raw[2];    /* ADC value in water (probe 1, probe 2)  */
    /* climate */
    int8_t   fan_on_c;           /* fan on at/above this temperature       */
    int8_t   heat_on_c;          /* heater on at/below this temperature    */
    uint8_t  hum_max;            /* fan on above this RH % (100 = disabled)*/
    /* lamp */
    uint8_t  lamp_start;         /* lamp allowed from this hour            */
    uint8_t  lamp_end;           /* ... until this hour (== start -> 24 h) */
    uint16_t light_low_lux;      /* lamp ON below this many lux            */
    uint16_t light_high_lux;     /* lamp OFF above this many lux           */
    uint8_t  crc;                /* MUST stay the last field               */
} settings_t;

extern settings_t g_settings;    /* the active settings                    */

static inline bool feature_on(uint16_t feat)
{
    return (g_settings.features & feat) != 0;
}

void settings_defaults(settings_t *s);          /* fill with factory defaults    */
bool settings_check(const settings_t *s);       /* range check only              */
bool settings_store(const settings_t *s);       /* check -> activate -> EEPROM   */
void settings_load(void);                       /* EEPROM -> g_settings (or defaults) */

#endif
