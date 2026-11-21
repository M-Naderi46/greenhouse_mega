#ifndef JALALI_H
#define JALALI_H

#include <stdbool.h>
#include <stdint.h>

/*
 * Gregorian <-> Jalali (Solar Hijri, Persian) calendar conversion.
 *
 * Exact integer algorithm (Borkowski's leap-year break table, as used by
 * jalaali-js). It matches the official Iranian calendar for years 1178..1633
 * AP; the RTC range 2000..2099 AD (1378..1478 AP) is well inside it.
 */
typedef struct {
    uint16_t y;      /* e.g. 1405 */
    uint8_t  m;      /* 1..12     */
    uint8_t  d;      /* 1..31     */
} jdate_t;

bool    jalali_from_gregorian(uint16_t gy, uint8_t gm, uint8_t gd, jdate_t *j);
bool    jalali_to_gregorian(const jdate_t *j, uint16_t *gy, uint8_t *gm, uint8_t *gd);
bool    jalali_is_leap(uint16_t jy);                 /* Esfand has 30 days */
uint8_t jalali_month_days(uint16_t jy, uint8_t jm);  /* 29..31 */
const char *jalali_month_name(uint8_t jm);           /* "Farvardin" ... "Esfand" */

#endif
