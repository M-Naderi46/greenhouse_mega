#include "user_config.h"
#include <stdio.h>
#include "settings_menu.h"
#include "settings.h"
#include "buttons.h"
#include "lcd_i2c.h"
#include "rtc_ds3231.h"
#include "jalali.h"
#include "sensors.h"
#include "irrigation.h"
#include "climate.h"
#include "outputs.h"
#include "air_sensor.h"
#include "hal_timebase.h"

/* Items are grouped in sections. MODE = next item, hold MODE = next section. */
enum {
    /* FEATURES: two presets first, then single switches (UP = ON, DOWN = OFF) */
    M_P_PLACE, M_P_WATER, M_AIR,
    M_F_IRRIG, M_F_SOIL2, M_F_LIGHT, M_F_TANK, M_F_PUMP,
    M_F_FAN, M_F_HEATER, M_F_LAMP, M_F_LOG,
    /* IRRIGATION */
    M_LOW, M_HIGH, M_WSTART, M_WEND, M_MAXL, M_MAXRUN, M_COOL, M_MINTEMP, M_PPL,
    /* SOIL CAL */
    M_CAL1DRY, M_CAL1WET, M_CAL2DRY, M_CAL2WET,
    /* CLIMATE */
    M_FANON, M_HEATON, M_HUMMAX,
    /* LAMP */
    M_LAMPSTART, M_LAMPEND, M_LIGHTLOW, M_LIGHTHIGH,
    /* CLOCK */
    M_HOUR, M_MIN, M_DAY, M_MON, M_YEAR,
    /* TOOLS */
    M_TESTVALVE, M_TESTFAN, M_TESTHEAT, M_TESTLAMP, M_DEFAULTS, M_SAVE,
    M_COUNT
};

static const char *const names[M_COUNT] = {
    "Place: open/greenhs", "Water source",       "Air temp/hum sensor",
    "Auto watering",      "Soil probe 2",        "Light sensor BH1750",
    "Tank float switch",   "Water pump",          "Fan control",
    "Heater control",      "Grow lamp control",   "UART data log",
    "Start below soil %",  "Stop above soil %",   "Window start hour",
    "Window end hour",     "Max litres / cycle",  "Max run (minutes)",
    "Pause between (min)", "No water below C",    "Flow pulses/litre",
    "CAL probe1: in AIR",  "CAL probe1: in WATER",
    "CAL probe2: in AIR",  "CAL probe2: in WATER",
    "Fan ON at temp",      "Heater ON at temp",   "Fan ON above hum %",
    "Lamp from hour",      "Lamp until hour",
    "Lamp ON below lux",   "Lamp OFF above lux",
    "Clock: hour",         "Clock: minute",
#if DATE_JALALI
    "Shamsi date: day",    "Shamsi date: month",  "Shamsi date: year",
#else
    "Date: day",           "Date: month",         "Date: year (20xx)",
#endif
    "Test valve",          "Test fan",            "Test heater",
    "Test lamp",           "Restore defaults",    "Save and exit"
};

/* feature bit for each of the M_F_* items (same order, index = item - M_F_IRRIG) */
static const uint16_t feat_bit[M_F_LOG - M_F_IRRIG + 1] = {
    FEAT_IRRIG, FEAT_SOIL2, FEAT_LIGHT, FEAT_TANK, FEAT_PUMP,
    FEAT_FAN, FEAT_HEATER, FEAT_LAMP, FEAT_LOG
};

static bool is_switch(uint8_t i)
{
    return i >= M_F_IRRIG && i <= M_F_LOG;
}

enum { SEC_FEAT, SEC_IRR, SEC_SOIL, SEC_CLIMATE, SEC_LAMP, SEC_CLOCK, SEC_TOOLS, SEC_COUNT };
static const char *const sec_names[SEC_COUNT] = {
    "FEATURES", "IRRIGATION", "SOIL CAL", "CLIMATE", "LAMP", "CLOCK", "TOOLS"
};

static uint8_t section_of(uint8_t i)
{
    if (i <= M_F_LOG)      return SEC_FEAT;
    if (i <= M_PPL)        return SEC_IRR;
    if (i <= M_CAL2WET)    return SEC_SOIL;
    if (i <= M_HUMMAX)     return SEC_CLIMATE;
    if (i <= M_LIGHTHIGH)  return SEC_LAMP;
    if (i <= M_YEAR)       return SEC_CLOCK;
    return SEC_TOOLS;
}

static bool        active;
static uint8_t     item;
static uint32_t    t_last, msg_until;
static settings_t  edit;                  /* working copy, applied on SAVE */
static rtc_t       clk;
#if DATE_JALALI
#define JYEAR_MIN 1379                      /* 2000-03-20 .. 2099 (RTC range) */
#define JYEAR_MAX 1477
static jdate_t     jdt;                     /* Jalali copy of the RTC date */

static void jdt_load(void)
{
    if (!jalali_from_gregorian((uint16_t)(2000 + clk.year), clk.mon, clk.day, &jdt)) {
        jdt.y = JYEAR_MIN; jdt.m = 1; jdt.d = 1;
    }
}

/* Jalali -> Gregorian -> RTC (only valid dates are ever written) */
static void jdt_store(void)
{
    uint8_t md = jalali_month_days(jdt.y, jdt.m);
    if (jdt.d > md) jdt.d = md;                       /* e.g. 31 Mehr -> 30 Mehr */
    uint16_t gy; uint8_t gm, gd;
    if (jalali_to_gregorian(&jdt, &gy, &gm, &gd) && gy >= 2000 && gy <= 2099) {
        clk.year = (uint8_t)(gy - 2000); clk.mon = gm; clk.day = gd;
        rtc_write(&clk);
    }
}
#endif
static bool        confirm_defaults;
static const char *msg;

static bool feat(uint16_t f)
{
    return (edit.features & f) != 0;
}

/* items for functions that are switched off (in the edited settings) are skipped */
static bool item_on(uint8_t i)
{
    switch (i) {
    case M_CAL2DRY: case M_CAL2WET:                      return feat(FEAT_SOIL2);
    case M_FANON: case M_HUMMAX: case M_TESTFAN:         return feat(FEAT_FAN);
    case M_HEATON: case M_TESTHEAT:                      return feat(FEAT_HEATER);
    case M_LAMPSTART: case M_LAMPEND: case M_LIGHTLOW:
    case M_LIGHTHIGH: case M_TESTLAMP:                   return feat(FEAT_LAMP) && feat(FEAT_LIGHT);
    default:                                             return true;
    }
}

static uint8_t next_item(uint8_t i)
{
    do { i++; } while (i < M_COUNT - 1 && !item_on(i));
    return i;
}

static uint8_t next_section(uint8_t i)
{
    uint8_t sec = section_of(i);
    for (uint8_t j = (uint8_t)(i + 1); j < M_COUNT; j++)
        if (item_on(j) && section_of(j) != sec) return j;
    return M_P_PLACE;                                    /* wrap around */
}

static int clampi(int v, int lo, int hi)
{
    return v < lo ? lo : (v > hi ? hi : v);
}

static uint8_t wrap(int v, int lo, int hi)
{
    if (v > hi) return (uint8_t)lo;
    if (v < lo) return (uint8_t)hi;
    return (uint8_t)v;
}

bool menu_active(void)
{
    return active;
}

static void release_tests(void)
{
    irrigation_manual(false);
    climate_release();
}

void menu_enter(void)
{
    edit = g_settings;
    if (!rtc_read(&clk)) clk = sensors_get()->time;
#if DATE_JALALI
    jdt_load();
#endif
    item = M_P_PLACE;
    confirm_defaults = false;
    msg = 0;
    t_last = millis();
    active = true;
}

static void menu_exit(void)
{
    release_tests();
    active = false;
}

static void menu_change(int dir)
{
    uint8_t rep = buttons_repeat_count();
    int fast = (rep > 8) ? 5 : 1;
    const sensors_t *s = sensors_get();

    if (item == M_P_PLACE) {                     /* preset: open field / greenhouse */
        if (dir > 0) edit.features &= (uint16_t)~FEAT_CLIMATE;   /* no fan, heater, lamp */
        else         edit.features |= FEAT_CLIMATE;
        return;
    }
    if (item == M_P_WATER) {                     /* preset: tank + pump / mains */
        if (dir > 0) edit.features |= (FEAT_TANK | FEAT_PUMP);
        else         edit.features &= (uint16_t)~(FEAT_TANK | FEAT_PUMP);
        return;
    }
    if (item == M_AIR) {                         /* air sensor type: DHT11 / DHT12 / AHT20 */
        edit.air_sensor = (uint8_t)clampi(edit.air_sensor + dir, 0, AIR_COUNT - 1);
        return;
    }
    if (is_switch(item)) {                       /* single feature switches */
        uint16_t bit = feat_bit[item - M_F_IRRIG];
        if (dir > 0) edit.features |= bit; else edit.features &= (uint16_t)~bit;
        return;
    }

    switch (item) {
    /* irrigation */
    case M_LOW:     edit.soil_low  = (uint8_t)clampi(edit.soil_low + dir, 0, edit.soil_high - 1); break;
    case M_HIGH:    edit.soil_high = (uint8_t)clampi(edit.soil_high + dir, edit.soil_low + 1, 99); break;
    case M_WSTART:  edit.win_start = wrap(edit.win_start + dir, 0, 23); break;
    case M_WEND:    edit.win_end   = wrap(edit.win_end + dir, 0, 23); break;
    case M_MAXL:    edit.max_liters   = (uint8_t)clampi(edit.max_liters + dir * fast, 1, 99); break;
    case M_MAXRUN:  edit.max_run_min  = (uint8_t)clampi(edit.max_run_min + dir * fast, 1, 60); break;
    case M_COOL:    edit.cooldown_min = (uint8_t)clampi(edit.cooldown_min + dir * fast, 0, 240); break;
    case M_MINTEMP: edit.min_temp_c   = (int8_t)clampi(edit.min_temp_c + dir, -10, 15); break;
    case M_PPL: {
        int step = (rep > 25) ? 100 : (rep > 8 ? 10 : 1);
        edit.pulses_per_liter = (uint16_t)clampi(edit.pulses_per_liter + dir * step, 10, 9999);
        break;
    }

    /* soil calibration: UP stores the current reading */
    case M_CAL1DRY: if (dir > 0) edit.soil_dry_raw[0] = s->soil_raw[0]; break;
    case M_CAL1WET: if (dir > 0) edit.soil_wet_raw[0] = s->soil_raw[0]; break;
    case M_CAL2DRY: if (dir > 0) edit.soil_dry_raw[1] = s->soil_raw[1]; break;
    case M_CAL2WET: if (dir > 0) edit.soil_wet_raw[1] = s->soil_raw[1]; break;

    /* climate (fan threshold always stays >= 6 degrees above heater threshold) */
    case M_FANON:   edit.fan_on_c  = (int8_t)clampi(edit.fan_on_c + dir, edit.heat_on_c + 6, 50); break;
    case M_HEATON:  edit.heat_on_c = (int8_t)clampi(edit.heat_on_c + dir, -5, edit.fan_on_c - 6); break;
    case M_HUMMAX:  edit.hum_max   = (uint8_t)clampi(edit.hum_max + dir * fast, 40, 100); break;

    /* lamp */
    case M_LAMPSTART: edit.lamp_start = wrap(edit.lamp_start + dir, 0, 23); break;
    case M_LAMPEND:   edit.lamp_end   = wrap(edit.lamp_end + dir, 0, 23); break;
    case M_LIGHTLOW:
    case M_LIGHTHIGH: {
        int step = (rep > 25) ? 2000 : (rep > 8 ? 500 : 100);
        if (item == M_LIGHTLOW)
            edit.light_low_lux  = (uint16_t)clampi(edit.light_low_lux + dir * step, 0, edit.light_high_lux - 100);
        else
            edit.light_high_lux = (uint16_t)clampi(edit.light_high_lux + dir * step, edit.light_low_lux + 100, 60000);
        break;
    }

    /* clock is written to the RTC immediately */
    case M_HOUR: clk.hour = wrap(clk.hour + dir, 0, 23);  clk.sec = 0; rtc_write(&clk); break;
    case M_MIN:  clk.min  = wrap(clk.min + dir, 0, 59);   clk.sec = 0; rtc_write(&clk); break;
#if DATE_JALALI
    case M_DAY:  jdt.d = wrap(jdt.d + dir, 1, jalali_month_days(jdt.y, jdt.m)); jdt_store(); break;
    case M_MON:  jdt.m = wrap(jdt.m + dir, 1, 12); jdt_store(); break;
    case M_YEAR: {
        int y = (int)jdt.y + dir;
        if (y > JYEAR_MAX) y = JYEAR_MIN;
        if (y < JYEAR_MIN) y = JYEAR_MAX;
        jdt.y = (uint16_t)y;
        jdt_store();                                  /* Esfand 30 -> 29 in a common year */
        break;
    }
#else
    case M_DAY:  clk.day  = wrap(clk.day + dir, 1, 31);   rtc_write(&clk); break;
    case M_MON:  clk.mon  = wrap(clk.mon + dir, 1, 12);   rtc_write(&clk); break;
    case M_YEAR: clk.year = wrap(clk.year + dir, 0, 99);  rtc_write(&clk); break;
#endif

    /* tests: UP = on, DOWN = off, released when leaving the item */
    case M_TESTVALVE: irrigation_manual(dir > 0); break;
    case M_TESTFAN:   climate_force(OUT_FAN,    dir > 0); break;
    case M_TESTHEAT:  climate_force(OUT_HEATER, dir > 0); break;
    case M_TESTLAMP:  climate_force(OUT_LAMP,   dir > 0); break;

    case M_DEFAULTS:
        if (dir > 0) {
            if (confirm_defaults) {
                settings_defaults(&edit);
                confirm_defaults = false;
                msg = "Defaults loaded";
                msg_until = millis() + 2000;
            } else {
                confirm_defaults = true;
            }
        }
        break;

    default: break;
    }
}

void menu_key(uint8_t ev)
{
    t_last = millis();
    if (ev & (KEY_MODE | KEY_MODE_LONG | KEY_DOWN)) confirm_defaults = false;

    if (ev & KEY_MODE_LONG) {                   /* jump to the next section */
        release_tests();
        item = next_section(item);
        return;
    }

    if (ev & KEY_MODE) {
        if (item == M_SAVE) {
            if (settings_store(&edit)) {
                menu_exit();
            } else {
                msg = "Invalid! check CAL";
                msg_until = millis() + 3000;
                item = M_CAL1DRY;
            }
            return;
        }
        release_tests();
        item = next_item(item);
        return;
    }

    if (item == M_SAVE) {                       /* DOWN = discard changes */
        if (ev & KEY_DOWN) menu_exit();
        return;
    }
    if (ev & KEY_UP)   menu_change(1);
    if (ev & KEY_DOWN) menu_change(-1);
}

void menu_task(void)
{
    if (active && millis() - t_last > MENU_TIMEOUT_MS) menu_exit();   /* discard */
}

static const char *hours_suffix(uint8_t a, uint8_t b)
{
    return a == b ? "(24h)" : "";
}

void menu_draw(void)
{
    char b[32], now_lux[8];
    const sensors_t *s = sensors_get();
    uint8_t pos = 0, total = 0;

    for (uint8_t i = 0; i < M_COUNT; i++) {
        if (!item_on(i)) continue;
        total++;
        if (i <= item) pos = total;
    }
    snprintf(b, sizeof b, "%-10s %2u/%-2u", sec_names[section_of(item)], pos, total);
    lcd_line(0, b);
    lcd_line(1, names[item]);

    if (s->light_ok) snprintf(now_lux, sizeof now_lux, "%u", s->light_lux);
    else             snprintf(now_lux, sizeof now_lux, "--");

    b[0] = 0;
    if (item == M_P_PLACE) {
        uint16_t cl = edit.features & FEAT_CLIMATE;
        snprintf(b, sizeof b, "%s", cl == 0 ? "OPEN FIELD" :
                                    (cl == FEAT_CLIMATE ? "GREENHOUSE" : "custom"));
    } else if (item == M_P_WATER) {
        uint16_t w = edit.features & (FEAT_TANK | FEAT_PUMP);
        snprintf(b, sizeof b, "%s", w == (FEAT_TANK | FEAT_PUMP) ? "TANK + PUMP" :
                                    (w == 0 ? "MAINS (no pump)" : "custom"));
    } else if (item == M_AIR) {
        static const char *const air_names[AIR_COUNT] = {
            "DHT11 (1-wire)", "DHT12 (I2C 0x5C)", "AHT20 (I2C 0x38)"
        };
        snprintf(b, sizeof b, "%s", air_names[edit.air_sensor < AIR_COUNT ? edit.air_sensor : 0]);
    } else if (is_switch(item)) {
        snprintf(b, sizeof b, "%s", (edit.features & feat_bit[item - M_F_IRRIG]) ? "ON" : "OFF");
    } else {
        switch (item) {
        case M_LOW:     snprintf(b, sizeof b, "%u %%", edit.soil_low); break;
        case M_HIGH:    snprintf(b, sizeof b, "%u %%", edit.soil_high); break;
        case M_WSTART:  snprintf(b, sizeof b, "%02u:00", edit.win_start); break;
        case M_WEND:    snprintf(b, sizeof b, "%02u:00 %s", edit.win_end,
                                 hours_suffix(edit.win_start, edit.win_end)); break;
        case M_MAXL:    snprintf(b, sizeof b, "%u L", edit.max_liters); break;
        case M_MAXRUN:  snprintf(b, sizeof b, "%u min", edit.max_run_min); break;
        case M_COOL:    snprintf(b, sizeof b, "%u min", edit.cooldown_min); break;
        case M_MINTEMP: snprintf(b, sizeof b, "%d C", edit.min_temp_c); break;
        case M_PPL:     snprintf(b, sizeof b, "%u", edit.pulses_per_liter); break;

        case M_CAL1DRY: snprintf(b, sizeof b, "Now:%4u Set:%4u", s->soil_raw[0], edit.soil_dry_raw[0]); break;
        case M_CAL1WET: snprintf(b, sizeof b, "Now:%4u Set:%4u", s->soil_raw[0], edit.soil_wet_raw[0]); break;
        case M_CAL2DRY: snprintf(b, sizeof b, "Now:%4u Set:%4u", s->soil_raw[1], edit.soil_dry_raw[1]); break;
        case M_CAL2WET: snprintf(b, sizeof b, "Now:%4u Set:%4u", s->soil_raw[1], edit.soil_wet_raw[1]); break;

        case M_FANON:   case M_HEATON: {
            char tn[10];
            if (s->air_ok) sensors_fmt_temp(tn, sizeof tn, s->air_t10); else snprintf(tn, sizeof tn, "--");
            snprintf(b, sizeof b, "%d C (now %sC)", item == M_FANON ? edit.fan_on_c : edit.heat_on_c, tn);
            break;
        }
        case M_HUMMAX:
            if (edit.hum_max >= 100) snprintf(b, sizeof b, "off");
            else                     snprintf(b, sizeof b, "%u %%", edit.hum_max);
            break;

        case M_LAMPSTART: snprintf(b, sizeof b, "%02u:00", edit.lamp_start); break;
        case M_LAMPEND:   snprintf(b, sizeof b, "%02u:00 %s", edit.lamp_end,
                                   hours_suffix(edit.lamp_start, edit.lamp_end)); break;
        case M_LIGHTLOW:  snprintf(b, sizeof b, "%u lx (now %s)", edit.light_low_lux, now_lux); break;
        case M_LIGHTHIGH: snprintf(b, sizeof b, "%u lx (now %s)", edit.light_high_lux, now_lux); break;

        case M_HOUR:    snprintf(b, sizeof b, "%02u", clk.hour); break;
        case M_MIN:     snprintf(b, sizeof b, "%02u", clk.min); break;
#if DATE_JALALI
        case M_DAY:     snprintf(b, sizeof b, "%02u", jdt.d); break;
        case M_MON:     snprintf(b, sizeof b, "%02u %s", jdt.m, jalali_month_name(jdt.m)); break;
        case M_YEAR:    snprintf(b, sizeof b, "%04u", jdt.y); break;
#else
        case M_DAY:     snprintf(b, sizeof b, "%02u", clk.day); break;
        case M_MON:     snprintf(b, sizeof b, "%02u", clk.mon); break;
        case M_YEAR:    snprintf(b, sizeof b, "20%02u", clk.year); break;
#endif

        case M_TESTVALVE:
            if (feat(FEAT_PUMP))
                snprintf(b, sizeof b, "V:%s Pump:%s", irrigation_is_open() ? "OPEN" : "closed",
                         outputs_get(OUT_PUMP) ? "ON" : "off");
            else
                snprintf(b, sizeof b, "Valve: %s", irrigation_is_open() ? "OPEN" : "closed");
            break;
        case M_TESTFAN:   snprintf(b, sizeof b, "Fan: %s", outputs_get(OUT_FAN) ? "ON" : "off"); break;
        case M_TESTHEAT:  snprintf(b, sizeof b, "Heater: %s", outputs_get(OUT_HEATER) ? "ON" : "off"); break;
        case M_TESTLAMP:  snprintf(b, sizeof b, "Lamp: %s", outputs_get(OUT_LAMP) ? "ON" : "off"); break;
        case M_DEFAULTS:  snprintf(b, sizeof b, "%s", confirm_defaults ? "Sure?" : ""); break;
        default: break;
        }
    }
    lcd_line(2, b);

    if (msg && millis() < msg_until) {
        lcd_line(3, msg);
        return;
    }
    if (item == M_P_PLACE) { lcd_line(3, "UP=open DN=greenhs"); return; }
    if (item == M_P_WATER) { lcd_line(3, "UP=tank DN=mains");   return; }
    if (item == M_AIR)     { lcd_line(3, "UP/DN=type then Save"); return; }
    if (item == M_F_LAMP)  { lcd_line(3, "Needs light sensor");  return; }
    if (item == M_F_PUMP)  {
        lcd_line(3, feat(FEAT_TANK) ? "UP=ON DN=OFF" : "Add tank switch!");
        return;
    }
    if (is_switch(item)) { lcd_line(3, "UP=ON DN=OFF"); return; }
    switch (item) {
    case M_CAL1DRY: case M_CAL1WET: case M_CAL2DRY: case M_CAL2WET:
                     lcd_line(3, "UP=store MODE=next"); break;
    case M_TESTVALVE: case M_TESTFAN: case M_TESTHEAT: case M_TESTLAMP:
                     lcd_line(3, "UP=on DN=off"); break;
    case M_DEFAULTS: lcd_line(3, confirm_defaults ? "UP again=confirm" : "UP=restore MODE=next"); break;
    case M_SAVE:     lcd_line(3, "MODE=save DN=discard"); break;
    default:         lcd_line(3, "MODE=next UP/DN=chg"); break;
    }
}
