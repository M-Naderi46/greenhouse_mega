#include "user_config.h"
#include <stdio.h>
#include <stdbool.h>
#include "ui.h"
#include "settings_menu.h"
#include "settings.h"
#include "buttons.h"
#include "lcd_i2c.h"
#include "sensors.h"
#include "irrigation.h"
#include "climate.h"
#include "outputs.h"
#include "hal_timebase.h"
#include "jalali.h"

static uint32_t t_draw, t_reinit, t_page;
static bool     redraw = true;
static uint8_t  page;                 /* 0 = measurements, 1 = outputs */

void ui_init(void)
{
    t_draw = t_reinit = t_page = millis();
}

static const char *out_text(out_id_t id)
{
    if (!climate_enabled(id)) return "n/a";
    return outputs_get(id) ? "ON " : "off";
}

/* line 4: the most important status of the whole system */
static void draw_status(void)
{
    char b[32];
    const sensors_t *s = sensors_get();
    irr_fault_t f = irrigation_fault();

    if (f == IRR_FAULT_NO_FLOW) {
        lcd_line(3, "!NO FLOW! UP=reset");
    } else if (f == IRR_FAULT_LEAK) {
        lcd_line(3, "!WATER LEAK! UP=rst");
    } else if (s->tank_empty) {
        lcd_line(3, "!TANK EMPTY!");
    } else if (irrigation_is_open()) {
        uint32_t c = irrigation_cycle_liters10(), r = irrigation_lpm10();
        snprintf(b, sizeof b, "ON %lu.%luL %lu.%luL/m",
                 (unsigned long)(c / 10), (unsigned long)(c % 10),
                 (unsigned long)(r / 10), (unsigned long)(r % 10));
        lcd_line(3, b);
    } else {
        uint32_t d = irrigation_day_liters10();
        snprintf(b, sizeof b, "%s Today:%lu.%luL",
                 feature_on(FEAT_IRRIG) ? "OFF " : "MAN ",
                 (unsigned long)(d / 10), (unsigned long)(d % 10));
        lcd_line(3, b);
    }
}

static void draw_page_measure(void)
{
    const sensors_t *s = sensors_get();
    char b[32], t[8], h[8], lx[12], a[8], c[8];

    if (s->rtc_ok) {
#if DATE_JALALI
        jdate_t j;
        if (jalali_from_gregorian((uint16_t)(2000 + s->time.year), s->time.mon, s->time.day, &j))
            snprintf(b, sizeof b, "%02u:%02u:%02u %04u/%02u/%02u",
                     s->time.hour, s->time.min, s->time.sec, j.y, j.m, j.d);
        else
            snprintf(b, sizeof b, "%02u:%02u:%02u  DATE ERR",
                     s->time.hour, s->time.min, s->time.sec);
#else
        snprintf(b, sizeof b, "%02u:%02u:%02u %02u/%02u/20%02u",
                 s->time.hour, s->time.min, s->time.sec,
                 s->time.day, s->time.mon, s->time.year);
#endif
    } else
        snprintf(b, sizeof b, "RTC ERROR");
    lcd_line(0, b);

    if (s->air_ok) {
        sensors_fmt_temp(t, sizeof t, s->air_t10);
        snprintf(h, sizeof h, "%u", s->air_h);
    } else {
        snprintf(t, sizeof t, "--.-");
        snprintf(h, sizeof h, "--");
    }
    lx[0] = 0;
    if (feature_on(FEAT_LIGHT)) {
        if (s->light_ok) snprintf(lx, sizeof lx, " L%u", s->light_lux);
        else             snprintf(lx, sizeof lx, " L--");
    }
    snprintf(b, sizeof b, "T%sC H%s%%%s", t, h, lx);
    lcd_line(1, b);

    if (s->soil_count > 1) {
        if (s->soil_valid[0]) snprintf(a, sizeof a, "%u", s->soil_p[0]); else snprintf(a, sizeof a, "--");
        if (s->soil_valid[1]) snprintf(c, sizeof c, "%u", s->soil_p[1]); else snprintf(c, sizeof c, "--");
        if (s->soil_ok)
            snprintf(b, sizeof b, "Soil:%u%% (%s|%s)", s->soil_pct, a, c);
        else
            snprintf(b, sizeof b, "SOIL SENSORS ERROR");
    } else {
        if (s->soil_ok)
            snprintf(b, sizeof b, "Soil:%3u%% L:%u H:%u",
                     s->soil_pct, g_settings.soil_low, g_settings.soil_high);
        else
            snprintf(b, sizeof b, "SOIL SENSOR ERROR");
    }
    lcd_line(2, b);
}

static void draw_page_outputs(void)
{
    const sensors_t *s = sensors_get();
    char b[32];

    snprintf(b, sizeof b, "Valve:%s Pump:%s", irrigation_is_open() ? "ON " : "off",
             out_text(OUT_PUMP));
    lcd_line(0, b);

    snprintf(b, sizeof b, "Fan:%s Heat:%s", out_text(OUT_FAN), out_text(OUT_HEATER));
    lcd_line(1, b);

    snprintf(b, sizeof b, "Lamp:%s Tank:%s", out_text(OUT_LAMP),
             feature_on(FEAT_TANK) ? (s->tank_empty ? "EMPTY" : "OK") : "n/a");
    lcd_line(2, b);
}

static void draw_main(void)
{
    if (page == 0) draw_page_measure(); else draw_page_outputs();
    draw_status();
}

void ui_task(void)
{
    uint32_t now = millis();
    uint8_t ev = buttons_poll();

    if (ev) {
        if (menu_active()) {
            menu_key(ev);
        } else {
            if (ev & (KEY_MODE | KEY_MODE_LONG)) menu_enter();
            if (ev & KEY_DOWN) { page ^= 1; t_page = now; }
            if ((ev & KEY_UP) && irrigation_fault() != IRR_FAULT_NONE)
                irrigation_clear_fault();
        }
        redraw = true;
    }
    menu_task();

    if (page && now - t_page > PAGE_RETURN_MS) { page = 0; redraw = true; }

    if (now - t_reinit >= LCD_REINIT_MS) {      /* recover from I2C glitches */
        t_reinit = now;
        lcd_init();
        redraw = true;
    }

    if (redraw || now - t_draw >= LCD_REFRESH_MS) {
        t_draw = now;
        redraw = false;
        if (menu_active()) menu_draw(); else draw_main();
    }
}
