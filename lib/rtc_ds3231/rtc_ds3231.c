#include "user_config.h"
#include "rtc_ds3231.h"
#include "hal_twi.h"

static uint8_t bcd2bin(uint8_t v) { return (uint8_t)((v >> 4) * 10 + (v & 0x0F)); }
static uint8_t bin2bcd(uint8_t v) { return (uint8_t)(((v / 10) << 4) | (v % 10)); }

bool rtc_read(rtc_t *t)
{
    uint8_t reg = 0, r[7];
    rtc_t tmp;

    if (!twi_write(RTC_I2C_ADDR, &reg, 1, false)) return false;
    if (!twi_read(RTC_I2C_ADDR, r, 7)) return false;

    tmp.sec  = bcd2bin(r[0] & 0x7F);
    tmp.min  = bcd2bin(r[1] & 0x7F);
    tmp.hour = bcd2bin(r[2] & 0x3F);
    tmp.day  = bcd2bin(r[4] & 0x3F);
    tmp.mon  = bcd2bin(r[5] & 0x1F);
    tmp.year = bcd2bin(r[6]);

    if (tmp.hour > 23 || tmp.min > 59 || tmp.sec > 59 ||
        tmp.day < 1 || tmp.day > 31 || tmp.mon < 1 || tmp.mon > 12)
        return false;

    *t = tmp;
    return true;
}

bool rtc_write(const rtc_t *t)
{
    uint8_t b[8] = { 0,
                     bin2bcd(t->sec), bin2bcd(t->min), bin2bcd(t->hour),
                     1,
                     bin2bcd(t->day), bin2bcd(t->mon), bin2bcd(t->year) };
    return twi_write(RTC_I2C_ADDR, b, 8, true);
}
