/*
 * Greenhouse controller - Arduino Mega 2560 (ATmega2560), bare-metal C
 *
 * All pins and options are in include/user_config.h.
 * Each device has its own library in lib/.
 */
#include "user_config.h"
#include <avr/wdt.h>
#include <util/delay.h>

#include "hal_system.h"
#include "hal_timebase.h"
#include "hal_twi.h"
#include "hal_uart.h"
#include "valve.h"
#include "outputs.h"
#include "buzzer.h"
#include "buttons.h"
#include "flow_meter.h"
#include "analog.h"
#include "soil_sensor.h"
#include "lcd_i2c.h"
#include "settings.h"
#include "sensors.h"
#include "irrigation.h"
#include "climate.h"
#include "datalog.h"
#include "ui.h"

static bool alarm_active(void)
{
    if (irrigation_fault() != IRR_FAULT_NONE) return true;
    if (TANK_ALARM && sensors_get()->tank_empty) return true;   /* false if tank feature is off */
    return false;
}

int main(void)
{
    valve_init();                 /* valve closed and all relays off before anything else */
    outputs_init();
    buzzer_init();
    buttons_init();
    flow_init();
    timebase_init();
    analog_init();                /* reads ADC calibration: interrupts still off */
    soil_init();
    twi_init();
    uart_init();
    settings_load();
    sensors_init();
    climate_init();

    system_enable_interrupts();

    lcd_init();
    lcd_line(0, "  GREENHOUSE CTRL");
    lcd_line(1, "  Arduino Mega 2560");
    lcd_line(2, "   starting...");
    lcd_line(3, "");
    _delay_ms(1500);

    system_wdt_start();

    sensors_update();
    sensors_update_air();
    irrigation_init();
    ui_init();
    datalog_init();

    uint32_t t_sec = millis();
    uint32_t t_air = t_sec;

    for (;;) {
        wdt_reset();
        uint32_t now = millis();

        if (now - t_sec >= SENSOR_PERIOD_MS) {
            t_sec += SENSOR_PERIOD_MS;
            sensors_update();
            irrigation_update();
            climate_update();
            datalog_task();
        }
        if (now - t_air >= DHT_PERIOD_MS) {
            t_air = now;
            sensors_update_air();
        }

        ui_task();
        buzzer_task(alarm_active());
    }
}
