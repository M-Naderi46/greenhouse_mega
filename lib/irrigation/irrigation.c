#include "user_config.h"
#include "irrigation.h"
#include "settings.h"
#include "sensors.h"
#include "valve.h"
#include "outputs.h"
#include "flow_meter.h"
#include "hal_timebase.h"

static bool        manual;
static bool        ever_watered;
static irr_fault_t fault;
static uint32_t    start_ms, stop_ms, start_pulses, cycle_pulses;
static uint32_t    last_pulses, day_pulses, leak_pulses, lpm10_v;
static uint8_t     leak_idle, last_day = 0xFF;

static void water_start(uint32_t now, uint32_t p)
{
    valve_set(true);
    start_ms = now;
    start_pulses = p;
    cycle_pulses = 0;
}

static void water_stop(uint32_t now)
{
    outputs_set(OUT_PUMP, false);       /* pump first, then close the valve */
    valve_set(false);
    stop_ms = now;
    ever_watered = true;
    manual = false;
    leak_pulses = 0;
    leak_idle = 0;
}

static bool in_window(const sensors_t *s)
{
    const settings_t *c = &g_settings;
    if (!s->rtc_ok) return true;                  /* RTC failed: rely on soil only */
    if (c->win_start == c->win_end) return true;  /* 24 h */
    uint8_t h = s->time.hour;
    if (c->win_start < c->win_end) return h >= c->win_start && h < c->win_end;
    return h >= c->win_start || h < c->win_end;   /* window crosses midnight */
}

void irrigation_init(void)
{
    last_pulses = flow_pulses();
    stop_ms = millis();
}

void irrigation_update(void)
{
    const sensors_t  *s = sensors_get();
    const settings_t *c = &g_settings;
    uint32_t now = millis();
    uint32_t p = flow_pulses();
    uint32_t delta = p - last_pulses;

    last_pulses = p;
    day_pulses += delta;
    lpm10_v = (delta * 600UL) / c->pulses_per_liter;

    if (s->rtc_ok && s->time.day != last_day) {   /* new day: reset daily total */
        last_day = s->time.day;
        day_pulses = 0;
    }

    if (valve_is_open()) {
        uint32_t run = now - start_ms;
        uint32_t rp  = p - start_pulses;
        cycle_pulses = rp;

        if (s->tank_empty)                              /* no water: stop, no fault latch */
            water_stop(now);
        else if (!manual && !s->soil_ok)
            water_stop(now);
        else if (!manual && !feature_on(FEAT_IRRIG))    /* auto watering switched off */
            water_stop(now);
        else if (!manual && s->soil_pct >= c->soil_high)
            water_stop(now);
        else if (rp >= (uint32_t)c->max_liters * c->pulses_per_liter)
            water_stop(now);
        else if (run >= (uint32_t)c->max_run_min * 60000UL)
            water_stop(now);
        else if (run >= FLOW_CHECK_S * 1000UL && rp < MIN_FLOW_PULSES) {
            fault = IRR_FAULT_NO_FLOW;
            water_stop(now);
        }
    } else {
        /* leak detection: water moving while the valve is closed */
        if (now - stop_ms >= LEAK_GRACE_S * 1000UL) {
            if (delta) {
                leak_pulses += delta;
                leak_idle = 0;
            } else if (++leak_idle >= 60) {
                leak_pulses = 0;
                leak_idle = 0;
            }
            if (leak_pulses >= LEAK_PULSES && fault == IRR_FAULT_NONE)
                fault = IRR_FAULT_LEAK;
        }

        bool cooldown_ok = !ever_watered ||
                           (now - stop_ms >= (uint32_t)c->cooldown_min * 60000UL);
        bool freezing = s->air_ok && s->air_t <= c->min_temp_c;

        if (feature_on(FEAT_IRRIG) && fault == IRR_FAULT_NONE && !s->tank_empty && s->soil_ok &&
            s->soil_pct < c->soil_low && in_window(s) && cooldown_ok && !freezing)
            water_start(now, p);
    }

    /*
     * Pump (tank mode): runs only while the valve is open, starts a moment AFTER
     * the valve opened (no dead-heading) and never with an empty tank.
     * water_stop() switches it off together with the valve.
     */
    outputs_set(OUT_PUMP, valve_is_open() && feature_on(FEAT_PUMP) && !s->tank_empty &&
                          (now - start_ms) >= PUMP_START_DELAY_S * 1000UL);
}

bool irrigation_is_open(void)          { return valve_is_open(); }
irr_fault_t irrigation_fault(void)     { return fault; }

void irrigation_clear_fault(void)
{
    fault = IRR_FAULT_NONE;
    leak_pulses = 0;
}

void irrigation_manual(bool on)
{
    uint32_t now = millis();
    if (on) {
        if (!valve_is_open() && fault == IRR_FAULT_NONE && !sensors_get()->tank_empty) {
            manual = true;
            water_start(now, flow_pulses());
        }
    } else if (valve_is_open() && manual) {
        water_stop(now);
    }
}

uint32_t irrigation_lpm10(void)
{
    return lpm10_v;
}

uint32_t irrigation_cycle_liters10(void)
{
    return (cycle_pulses * 10UL) / g_settings.pulses_per_liter;
}

uint32_t irrigation_day_liters10(void)
{
    return (day_pulses * 10UL) / g_settings.pulses_per_liter;
}
