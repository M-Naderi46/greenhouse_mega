#ifndef SENSORS_H
#define SENSORS_H

#include <stdbool.h>
#include <stdint.h>
#include "rtc_ds3231.h"

typedef struct {
    rtc_t    time;          bool rtc_ok;

    uint8_t  soil_count;    /* 1 or 2 probes in use (FEAT_SOIL2)         */
    uint16_t soil_raw[2];   /* per probe                                 */
    uint8_t  soil_p[2];     /* per probe, %                              */
    bool     soil_valid[2];
    uint8_t  soil_pct;      /* combined value used for irrigation        */
    bool     soil_ok;       /* at least one probe works                  */

    uint16_t light_lux;     bool light_ok;   /* false: disabled or no answer */

    int16_t  air_t10;       /* air temperature, tenths of a degree C (245 = 24.5) */
    int8_t   air_t;         /* same, rounded to whole degrees (used by the control logic) */
    uint8_t  air_h;         bool air_ok;

    bool     tank_empty;    /* always false when FEAT_TANK is off        */
} sensors_t;

void sensors_init(void);
void sensors_update(void);        /* RTC + soil + light + tank (every 1 s) */
void sensors_update_air(void);    /* air temp/hum sensor       (every 3 s) */
const sensors_t *sensors_get(void);

/* "24.5", "-3.2", "0.4": temperature in tenths of a degree C as text (no unit) */
void sensors_fmt_temp(char *buf, unsigned size, int16_t t10);

#endif
