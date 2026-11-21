/*
 * ============================================================================
 *  user_config.h  -  pins, hardware options and factory defaults.
 *                    Target: Arduino Mega 2560 (ATmega2560, 16 MHz, 5 V)
 * ============================================================================
 *  1. Default feature switches  (can be changed later from the on-device menu)
 *  2. Pin assignment            (written as  "PORT-LETTER, BIT"  e.g.  A, 0)
 *  3. Timing
 *  4. Hardware options          (I2C bus, LCD, RTC, DHT, BH1750, ADC, UART)
 *  5. Factory default settings  (stored in EEPROM, editable from the menu)
 *  6. Safety limits and control behaviour
 *
 *  A pin is written as  "<port letter>, <bit>"  :  #define VALVE_PIN  A, 0
 *  The Arduino pin number is given in the comment (D22 = PA0 ...).
 *
 *  FIXED by the chip (cannot be moved, avoid them for other functions):
 *    I2C  : PD1 = SDA (Arduino D20), PD0 = SCL (Arduino D21)
 *    UART : USART0 = PE0 RX / PE1 TX (Arduino D0 / D1)  -> USB cable (PC)
 *    ADC  : PF0..PF7 = A0..A7
 *    Flow : external interrupt INT4..INT7 = PE4..PE7 (D2, D3, D5 ... see below)
 * ============================================================================
 */
#ifndef USER_CONFIG_H
#define USER_CONFIG_H

#include <avr/io.h>
#include <stdint.h>

#ifndef F_CPU
#define F_CPU 16000000UL          /* Arduino Mega crystal */
#endif

/* ========================================================================== */
/* 1. DEFAULT FEATURE SWITCHES (1 = on)                                       */
/*    Only the FACTORY value: afterwards use  menu > FEATURES  to change them */
/*    (stored in EEPROM, applied after "Save and exit").                      */
/* ========================================================================== */
#define DEFAULT_IRRIG_ENABLED   1   /* automatic watering                      */
#define DEFAULT_SOIL2_ENABLED   0   /* second soil moisture probe              */
#define DEFAULT_LIGHT_ENABLED   1   /* BH1750 light sensor                     */
#define DEFAULT_TANK_ENABLED    0   /* float switch in the water tank          */
#define DEFAULT_PUMP_ENABLED    0   /* water pump (tank mode); runs with the valve */
#define DEFAULT_FAN_ENABLED     1   /* ventilation fan relay                   */
#define DEFAULT_HEATER_ENABLED  1   /* heater relay                            */
#define DEFAULT_LAMP_ENABLED    1   /* grow-light relay (needs the light sensor) */
#define DEFAULT_LOG_ENABLED     1   /* CSV data on UART (USB / ESP)            */

/* ========================================================================== */
/* 2. PIN ASSIGNMENT                                                          */
/* ========================================================================== */

/* --- Solenoid valve driver (MOSFET / relay module) ----------------------- */
#define VALVE_PIN           A, 0          /* D22 */
#define VALVE_ACTIVE_HIGH   1             /* 1: pin HIGH = valve open        */

/* --- Climate relays ------------------------------------------------------- */
#define FAN_PIN             A, 1          /* D23 */
#define FAN_ACTIVE_HIGH     1

#define HEATER_PIN          A, 2          /* D24 */
#define HEATER_ACTIVE_HIGH  1

#define LAMP_PIN            A, 3          /* D25 */
#define LAMP_ACTIVE_HIGH    1

/* --- Water pump relay (tank mode) ------------------------------------------
 *  The pump runs only while the valve is open, and only if the tank is not
 *  empty (needs the tank float switch to protect the pump from dry running).
 */
#define PUMP_PIN            A, 4          /* D26 */
#define PUMP_ACTIVE_HIGH    1

/* NOTE: many cheap relay modules are ACTIVE-LOW (IN = LOW switches on).
 *       For them set  *_ACTIVE_HIGH 0.                                       */

/* --- Buzzer or alarm LED -------------------------------------------------- */
#define ALARM_PIN           A, 5          /* D27 */
#define ALARM_ACTIVE_HIGH   1

/* --- Flow meter pulse input ------------------------------------------------
 *  Must be an external-interrupt pin of PORTE:  INT4=PE4(D2) INT5=PE5(D3)
 *  INT6=PE6(no header) INT7=PE7(no header). FLOW_INT_NUM must match FLOW_PIN.
 *  (INT0..INT3 are the I2C / USART1 pins and are not supported here.)
 */
#define FLOW_PIN            E, 4          /* D2 */
#define FLOW_INT_NUM        4             /* 4..7 */

/* --- Tank float switch (to GND, internal pull-up) ------------------------- */
#define TANK_PIN            C, 0          /* D37 */
#define TANK_EMPTY_WHEN_OPEN 1            /* 1: contact open = tank empty
                                             (also a cut wire = "empty", fail-safe) */

/* --- DHT11 data pin (only used when the air sensor type is DHT11) ---------- */
#define DHT_PIN             L, 0          /* D49 (add a 4.7k..10k pull-up to 5 V) */

/* --- Push buttons (to GND, internal pull-up) ------------------------------ */
#define BTN_MODE_PIN        C, 1          /* D36 */
#define BTN_UP_PIN          C, 2          /* D35 */
#define BTN_DOWN_PIN        C, 3          /* D34 */
#define BTN_ACTIVE_LOW      1             /* 1: pressed = GND (internal pull-up) */

/* --- Soil probes (analog) ---------------------------------------------------
 *  Channel number 0..7  =  PF0..PF7  =  Arduino A0..A7
 */
#define SOIL1_PIN           0             /* A0: soil moisture probe 1           */
#define SOIL2_PIN           1             /* A1: soil moisture probe 2 (optional)*/

/* optional: power the soil probes only while measuring (saves the probes) -- */
#define SOIL_POWER_SWITCHED 0
#define SOIL_POWER_PIN      A, 7          /* D29 */

/* --- I2C bus (LCD + RTC + BH1750 + optional AHT20 / DHT12) -----------------
 *  Hardware TWI: SDA = PD1 (D20), SCL = PD0 (D21)  (fixed)
 */
#define I2C_HZ              100000UL

/* --- UART for data logging (TX only is used) ------------------------------
 *  USART0 = PE1 (TX0, D1) - it is also the USB serial port of the Mega.
 *  So:  PC (serial terminal, 9600 baud)  or  ESP8266/ESP32 on D1 (TX0).
 *  Unplug the ESP while uploading firmware (USB uses the same pins).
 */
#define UART_BAUD           9600UL

/* ========================================================================== */
/* 3. TIMING                                                                  */
/* ========================================================================== */
#define SENSOR_PERIOD_MS    1000UL        /* RTC + soil + light + logic      */
#define DHT_PERIOD_MS       3000UL        /* DHT11 needs >= 1 s              */
#define LCD_REFRESH_MS      500UL
#define LCD_REINIT_MS       600000UL      /* re-init LCD every 10 min        */
#define MENU_TIMEOUT_MS     60000UL       /* leave menu after 60 s idle      */
#define PAGE_RETURN_MS      30000UL       /* main screen returns to page 1   */
#define LOG_PERIOD_S        10            /* one CSV line every N seconds    */

/* ========================================================================== */
/* 4. HARDWARE OPTIONS                                                        */
/* ========================================================================== */

/* --- LCD 20x4 with PCF8574 backpack --------------------------------------- */
#define LCD_I2C_ADDR        0x27          /* PCF8574 = 0x27, PCF8574A = 0x3F */
#define LCD_COLS            20
#define LCD_ROWS            4
/* PCF8574 bit mapping (most backpacks: RS=P0 RW=P1 EN=P2 BL=P3, D4..D7=P4..P7) */
#define LCD_RS_BIT          0x01
#define LCD_EN_BIT          0x04
#define LCD_BL_BIT          0x08

/* --- RTC ------------------------------------------------------------------ */
#define RTC_I2C_ADDR        0x68          /* DS3231 / DS1307                 */
/* Date shown on the LCD and edited in the menu: 1 = Jalali (Shamsi), 0 = Gregorian.
 * The RTC itself always stores the Gregorian date (2000..2099).               */
#define DATE_JALALI         1

/* --- Light sensor BH1750 (GY-302 module) -----------------------------------
 *  ADDR pin low/open = 0x23, ADDR pin high = 0x5C (clashes with DHT12!)
 */
#define BH1750_I2C_ADDR     0x23

/* --- Air temperature / humidity sensor ---------------------------------------
 *  The TYPE (DHT11, DHT12 or AHT20) is chosen in the menu:
 *  FEATURES > "Air temp/hum sensor". Here only the I2C addresses.
 *  The factory default type is DEFAULT_AIR_SENSOR in section 5.
 */
#define DHT12_I2C_ADDR      0x5C
#define AHT20_I2C_ADDR      0x38          /* AHT20 / AHT21 */

/* --- ADC (10 bit) -----------------------------------------------------------
 *  Reference:  0x40 = AVCC (5 V, default)   0xC0 = internal 2.56 V
 *              0x00 = AREF pin
 */
#define ANALOG_REFSEL       0x40
#define ANALOG_MAX          1023          /* 10-bit full scale               */
#define CAL_MIN_SPAN        25            /* dry/wet calibration must differ by this much */
#define SOIL_RAW_MIN_VALID  15            /* below  -> probe shorted/open    */
#define SOIL_RAW_MAX_VALID  1010          /* above  -> probe missing         */

/* How two soil probes are combined:  0 = average,  1 = driest probe wins     */
#define SOIL_COMBINE_MODE   0

/* ========================================================================== */
/* 5. FACTORY DEFAULT SETTINGS  (editable later from the on-device menu)      */
/* ========================================================================== */
/* air sensor type: 0 = DHT11 (one-wire), 1 = DHT12 (I2C), 2 = AHT20 (I2C)   */
#define DEFAULT_AIR_SENSOR        2

/* irrigation */
#define DEFAULT_SOIL_LOW          35      /* % : start watering below        */
#define DEFAULT_SOIL_HIGH         60      /* % : stop watering above         */
#define DEFAULT_WIN_START         6       /* allowed hours: start            */
#define DEFAULT_WIN_END           20      /*                end (== start -> 24 h) */
#define DEFAULT_MAX_LITERS        10      /* litres per watering cycle       */
#define DEFAULT_MAX_RUN_MIN       10      /* minutes per watering cycle      */
#define DEFAULT_COOLDOWN_MIN      10      /* pause between cycles            */
#define DEFAULT_MIN_TEMP_C        3       /* no watering at/below this temp  */
#define DEFAULT_PULSES_PER_LITER  450     /* YF-S201 ~450 - calibrate!       */
#define DEFAULT_SOIL_DRY_RAW      600     /* ADC (0..1023) in air   - calibrate! */
#define DEFAULT_SOIL_WET_RAW      270     /* ADC (0..1023) in water - calibrate! */
/* climate */
#define DEFAULT_FAN_ON_C          30      /* fan on at/above this temp       */
#define DEFAULT_HEAT_ON_C         12      /* heater on at/below this temp    */
#define DEFAULT_HUM_MAX           85      /* fan on above this RH % (100 = off) */
/* lamp (BH1750 gives lux; overcast day ~ 1000-10000 lx, full sun ~ 100000 lx) */
#define DEFAULT_LAMP_START        6       /* lamp allowed from this hour     */
#define DEFAULT_LAMP_END          20      /* ... until this hour (== start -> 24 h) */
#define DEFAULT_LIGHT_LOW_LUX     5000    /* lamp ON  below this many lux    */
#define DEFAULT_LIGHT_HIGH_LUX    15000   /* lamp OFF above this many lux    */

/* ========================================================================== */
/* 6. SAFETY LIMITS AND CONTROL BEHAVIOUR                                     */
/* ========================================================================== */
#define FLOW_CHECK_S        10            /* after this many s flow must exist */
#define MIN_FLOW_PULSES     20            /* ... at least this many pulses   */
#define LEAK_GRACE_S        8             /* ignore pulses after closing     */
#define LEAK_PULSES         100           /* pulses with valve closed = leak */
/* ~4 s watchdog.  If the board keeps resetting after a watchdog reset, it has
 * the old Arduino bootloader: set 0 here (or burn a newer bootloader). */
#define WDT_ENABLED         1

#define CLIMATE_HYST_C      2             /* fan/heater hysteresis (deg C)   */
#define CLIMATE_HUM_HYST    5             /* humidity hysteresis (% RH)      */
#define CLIMATE_MIN_STATE_S 30            /* min time between fan/heater changes */
#define LAMP_MIN_STATE_S    300           /* min time between lamp changes   */
#define FAN_ON_SENSOR_FAIL  1             /* air sensor failed: 1 = run fan, heater is always off */

#define PUMP_START_DELAY_S  2             /* pump starts this long AFTER the valve opens */

#define TANK_DEBOUNCE_S     5             /* switch must be stable this long */
#define TANK_ALARM          1             /* buzzer while tank is empty      */

#endif /* USER_CONFIG_H */
