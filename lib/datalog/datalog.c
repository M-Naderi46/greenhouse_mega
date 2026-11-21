#include "user_config.h"
#include <stdio.h>
#include <string.h>
#include "datalog.h"
#include "hal_uart.h"
#include "settings.h"
#include "sensors.h"
#include "irrigation.h"
#include "outputs.h"

static uint8_t seconds;

void datalog_init(void)
{
    static const char hdr[] =
        "# GH,time,air_t,air_h,soil1,soil2,soil,lux,valve,pump,fan,heater,lamp,tank,day_l,fault\r\n";
    if (feature_on(FEAT_LOG)) uart_write(hdr, (uint8_t)(sizeof hdr - 1));
}

void datalog_task(void)
{
    if (!feature_on(FEAT_LOG)) { seconds = 0; return; }
    if (++seconds < LOG_PERIOD_S) return;
    seconds = 0;

    const sensors_t *s = sensors_get();
    char ts[32], at[6], ah[6], s2[6], lx[8], line[128];

    if (s->rtc_ok)
        snprintf(ts, sizeof ts, "20%02u-%02u-%02u %02u:%02u:%02u",
                 s->time.year, s->time.mon, s->time.day,
                 s->time.hour, s->time.min, s->time.sec);
    else
        snprintf(ts, sizeof ts, "no-rtc");

    if (s->air_ok) {
        sensors_fmt_temp(at, sizeof at, s->air_t10);
        snprintf(ah, sizeof ah, "%u", s->air_h);
    } else {
        strcpy(at, "-");
        strcpy(ah, "-");
    }

    if (s->soil_count > 1 && s->soil_valid[1]) snprintf(s2, sizeof s2, "%u", s->soil_p[1]);
    else                                       strcpy(s2, "-");

    if (s->light_ok) snprintf(lx, sizeof lx, "%u", s->light_lux);
    else             strcpy(lx, "-");

    uint32_t day = irrigation_day_liters10();
    int n = snprintf(line, sizeof line,
                     "GH,%s,%s,%s,%u,%s,%u,%s,%u,%u,%u,%u,%u,%u,%lu.%lu,%u\r\n",
                     ts, at, ah,
                     s->soil_valid[0] ? s->soil_p[0] : 0, s2,
                     s->soil_ok ? s->soil_pct : 0,
                     lx,
                     irrigation_is_open() ? 1 : 0,
                     outputs_get(OUT_PUMP) ? 1 : 0,
                     outputs_get(OUT_FAN) ? 1 : 0,
                     outputs_get(OUT_HEATER) ? 1 : 0,
                     outputs_get(OUT_LAMP) ? 1 : 0,
                     s->tank_empty ? 1 : 0,
                     (unsigned long)(day / 10), (unsigned long)(day % 10),
                     (unsigned)irrigation_fault());

    if (n > 0 && n < (int)sizeof line) uart_write(line, (uint8_t)n);
}
