#include "user_config.h"
#include <avr/interrupt.h>
#include <util/delay.h>
#include "air_sensor.h"
#include "air_decode.h"
#include "settings.h"
#include "hal_twi.h"
#include "gpio.h"

static bool aht_ready;            /* AHT20 initialised and calibrated */

void air_init(void)
{
    /* DHT11 data pin: input with pull-up (harmless while another type is used) */
    PIN_INPUT(DHT_PIN);
    PIN_HIGH(DHT_PIN);                   /* pull-up */
    aht_ready = false;            /* AHT20 is started on first use */
}

/* ------------------------------ DHT12 (I2C) ------------------------------ */
static bool dht12_read(int16_t *t, uint8_t *h)
{
    uint8_t reg = 0, d[5];
    if (!twi_write(DHT12_I2C_ADDR, &reg, 1, false)) return false;
    if (!twi_read(DHT12_I2C_ADDR, d, 5)) return false;
    if ((uint8_t)(d[0] + d[1] + d[2] + d[3]) != d[4]) return false;

    /* d[2] = integer part, d[3] = tenths (bit 7 = negative) */
    int16_t tv = (int16_t)(d[2] * 10 + (d[3] & 0x7F));
    if (d[3] & 0x80) tv = (int16_t)-tv;
    *t = tv;
    *h = d[0];
    return true;
}

/* ------------------------------ AHT20 (I2C) ------------------------------ */
static bool aht20_start(void)
{
    uint8_t st;

    _delay_ms(40);                               /* power-up time */
    if (!twi_read(AHT20_I2C_ADDR, &st, 1)) return false;
    if ((st & 0x18) != 0x18) {                   /* not calibrated: send init command */
        static const uint8_t init[3] = { 0xBE, 0x08, 0x00 };
        if (!twi_write(AHT20_I2C_ADDR, init, 3, true)) return false;
        _delay_ms(10);
    }
    return true;
}

static bool aht20_read(int16_t *t, uint8_t *h)
{
    static const uint8_t trigger[3] = { 0xAC, 0x33, 0x00 };
    uint8_t d[7];

    if (!aht_ready) {
        aht_ready = aht20_start();
        if (!aht_ready) return false;
    }

    if (!twi_write(AHT20_I2C_ADDR, trigger, 3, true)) { aht_ready = false; return false; }
    _delay_ms(80);                               /* measurement time (typ. 75 ms) */

    for (uint8_t i = 0; i < 5; i++) {
        if (!twi_read(AHT20_I2C_ADDR, d, 7)) { aht_ready = false; return false; }
        if (!(d[0] & 0x80)) break;               /* not busy any more */
        _delay_ms(10);
    }
    return aht20_decode(d, t, h);
}

/* ------------------------------ DHT11 (one-wire) ------------------------- */
static bool wait_level(bool level, uint8_t us)
{
    while (us--) {
        if (PIN_READ(DHT_PIN) == level) return true;
        _delay_us(1);
    }
    return false;
}

static bool dht11_read(int16_t *t, uint8_t *h)
{
    uint8_t d[5] = { 0, 0, 0, 0, 0 };
    bool ok = false;

    PIN_LOW(DHT_PIN);                    /* start signal: low >= 18 ms */
    PIN_OUTPUT(DHT_PIN);
    _delay_ms(20);

    uint8_t sreg = SREG;
    cli();                               /* timing critical part (~4 ms) */
    PIN_HIGH(DHT_PIN);
    _delay_us(30);
    PIN_INPUT(DHT_PIN);                  /* release the line (pull-up stays on) */

    do {
        if (!wait_level(false, 100)) break;
        if (!wait_level(true, 100))  break;
        if (!wait_level(false, 100)) break;

        uint8_t i;
        for (i = 0; i < 40; i++) {
            if (!wait_level(true, 100)) break;
            _delay_us(40);               /* '0' ~27 us, '1' ~70 us */
            d[i >> 3] <<= 1;
            if (PIN_READ(DHT_PIN)) d[i >> 3] |= 1;
            if (!wait_level(false, 100)) break;
        }
        if (i < 40) break;
        ok = true;
    } while (0);

    SREG = sreg;
    PIN_INPUT(DHT_PIN);
    PIN_HIGH(DHT_PIN);

    if (!ok) return false;
    if ((uint8_t)(d[0] + d[1] + d[2] + d[3]) != d[4]) return false;
    *h = d[0];
    *t = (int16_t)(d[2] * 10);                   /* DHT11: whole degrees only */
    return true;
}

bool air_read(int16_t *t, uint8_t *h)
{
    switch (g_settings.air_sensor) {
    case AIR_DHT12: return dht12_read(t, h);
    case AIR_AHT20: return aht20_read(t, h);
    default:        return dht11_read(t, h);
    }
}
