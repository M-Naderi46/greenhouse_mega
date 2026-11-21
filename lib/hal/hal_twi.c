#include "user_config.h"
#include <avr/io.h>
#include <util/delay.h>
#include "hal_twi.h"

/* status codes (TWSR & 0xF8) */
#define ST_START      0x08
#define ST_REP_START  0x10
#define ST_SLAW_ACK   0x18
#define ST_DATA_W_ACK 0x28
#define ST_SLAR_ACK   0x40
#define ST_DATA_R_ACK 0x50
#define ST_DATA_R_NAK 0x58

void twi_init(void)
{
    TWSR = 0;                                            /* prescaler 1 */
    TWBR = (uint8_t)((F_CPU / I2C_HZ - 16UL) / 2UL);     /* 72 -> 100 kHz */
    TWCR = _BV(TWEN);
}

static inline uint8_t status(void) { return TWSR & 0xF8; }

/* wait for TWINT; false on timeout (bus stuck) */
static bool wait_int(void)
{
    uint16_t t = 20000;
    while (!(TWCR & _BV(TWINT))) {
        if (--t == 0) return false;
        _delay_us(2);
    }
    return true;
}

static void bus_stop(void)
{
    TWCR = _BV(TWINT) | _BV(TWSTO) | _BV(TWEN);
    uint8_t t = 255;
    while ((TWCR & _BV(TWSTO)) && --t) _delay_us(2);
}

/* hard reset of the TWI block (after a timeout) */
static void bus_abort(void)
{
    TWCR = 0;
    twi_init();
}

static bool send_start(void)
{
    TWCR = _BV(TWINT) | _BV(TWSTA) | _BV(TWEN);
    if (!wait_int()) return false;
    uint8_t s = status();
    return s == ST_START || s == ST_REP_START;
}

static bool send_byte(uint8_t b, uint8_t expect)
{
    TWDR = b;
    TWCR = _BV(TWINT) | _BV(TWEN);
    if (!wait_int()) return false;
    return status() == expect;
}

bool twi_write(uint8_t addr, const uint8_t *buf, uint8_t len, bool stop)
{
    if (!send_start()) { bus_abort(); return false; }
    if (!send_byte((uint8_t)(addr << 1), ST_SLAW_ACK)) goto fail;
    for (uint8_t i = 0; i < len; i++)
        if (!send_byte(buf[i], ST_DATA_W_ACK)) goto fail;
    if (stop) bus_stop();
    return true;
fail:
    if (TWCR & _BV(TWINT)) bus_stop(); else bus_abort();
    return false;
}

bool twi_read(uint8_t addr, uint8_t *buf, uint8_t len)
{
    if (len == 0) return true;
    if (!send_start()) { bus_abort(); return false; }
    if (!send_byte((uint8_t)((addr << 1) | 1), ST_SLAR_ACK)) goto fail;

    for (uint8_t i = 0; i < len; i++) {
        bool last = (i == len - 1);
        TWCR = _BV(TWINT) | _BV(TWEN) | (last ? 0 : _BV(TWEA));
        if (!wait_int()) { bus_abort(); return false; }
        if (status() != (last ? ST_DATA_R_NAK : ST_DATA_R_ACK)) goto fail;
        buf[i] = TWDR;
    }
    bus_stop();
    return true;
fail:
    bus_stop();
    return false;
}
