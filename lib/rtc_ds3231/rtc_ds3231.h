#ifndef RTC_DS3231_H
#define RTC_DS3231_H

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    uint8_t sec, min, hour;   /* 24 h format */
    uint8_t day, mon, year;   /* year = 0..99 (20xx) */
} rtc_t;

bool rtc_read(rtc_t *t);          /* false on I2C error or invalid data */
bool rtc_write(const rtc_t *t);

#endif
