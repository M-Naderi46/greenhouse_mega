#include "jalali.h"

/* leap-year break years of the Jalali calendar */
static const int16_t breaks[] = {
    -61, 9, 38, 199, 426, 686, 756, 818, 1111, 1181, 1210,
    1635, 2060, 2097, 2192, 2262, 2324, 2394, 2456, 3178
};
#define NBREAKS ((int)(sizeof breaks / sizeof breaks[0]))

typedef struct { int leap; int gy; int march; } jalcal_t;

static int32_t idiv(int32_t a, int32_t b) { return a / b; }     /* truncating */
static int32_t imod(int32_t a, int32_t b) { return a - (a / b) * b; }

/* leap = 0 for a leap year, 1..3 years since the last leap year;
 * march = day of March (Gregorian) on which 1 Farvardin falls. */
static bool jalcal(int32_t jy, jalcal_t *r)
{
    int32_t gy = jy + 621;
    int32_t leap_j = -14;
    int32_t jp = breaks[0];
    int32_t jump = 0, n;

    if (jy < jp || jy >= breaks[NBREAKS - 1]) return false;

    for (int i = 1; i < NBREAKS; i++) {
        int32_t jm = breaks[i];
        jump = jm - jp;
        if (jy < jm) break;
        leap_j += idiv(jump, 33) * 8 + idiv(imod(jump, 33), 4);
        jp = jm;
    }
    n = jy - jp;

    leap_j += idiv(n, 33) * 8 + idiv(imod(n, 33) + 3, 4);
    if (imod(jump, 33) == 4 && jump - n == 4) leap_j += 1;

    int32_t leap_g = idiv(gy, 4) - idiv((idiv(gy, 100) + 1) * 3, 4) - 150;
    r->march = (int)(20 + leap_j - leap_g);
    r->gy = (int)gy;

    if (jump - n < 6) n = n - jump + idiv(jump + 4, 33) * 33;
    int32_t leap = imod(imod(n + 1, 33) - 1, 4);
    if (leap == -1) leap = 4;
    r->leap = (int)leap;
    return true;
}

/* Gregorian date -> Julian day number */
static int32_t g2d(int32_t gy, int32_t gm, int32_t gd)
{
    int32_t d = idiv((gy + idiv(gm - 8, 6) + 100100) * 1461, 4)
              + idiv(153 * imod(gm + 9, 12) + 2, 5) + gd - 34840408;
    d = d - idiv(idiv(gy + 100100 + idiv(gm - 8, 6), 100) * 3, 4) + 752;
    return d;
}

static void d2g(int32_t jdn, int32_t *gy, int32_t *gm, int32_t *gd)
{
    int32_t j = 4 * jdn + 139361631;
    j = j + idiv(idiv(4 * jdn + 183187720, 146097) * 3, 4) * 4 - 3908;
    int32_t i = idiv(imod(j, 1461), 4) * 5 + 308;
    *gd = idiv(imod(i, 153), 5) + 1;
    *gm = imod(idiv(i, 153), 12) + 1;
    *gy = idiv(j, 1461) - 100100 + idiv(8 - *gm, 6);
}

bool jalali_is_leap(uint16_t jy)
{
    jalcal_t r;
    return jalcal(jy, &r) && r.leap == 0;
}

uint8_t jalali_month_days(uint16_t jy, uint8_t jm)
{
    if (jm <= 6)  return 31;
    if (jm <= 11) return 30;
    return jalali_is_leap(jy) ? 30 : 29;
}

bool jalali_to_gregorian(const jdate_t *j, uint16_t *gy, uint8_t *gm, uint8_t *gd)
{
    jalcal_t r;
    if (j->m < 1 || j->m > 12 || j->d < 1 || j->d > jalali_month_days(j->y, j->m)) return false;
    if (!jalcal(j->y, &r)) return false;

    int32_t jdn = g2d(r.gy, 3, r.march)
                + (int32_t)(j->m - 1) * 31 - idiv(j->m, 7) * (j->m - 7) + j->d - 1;
    int32_t y, m, d;
    d2g(jdn, &y, &m, &d);
    *gy = (uint16_t)y; *gm = (uint8_t)m; *gd = (uint8_t)d;
    return true;
}

bool jalali_from_gregorian(uint16_t gy, uint8_t gm, uint8_t gd, jdate_t *out)
{
    if (gm < 1 || gm > 12 || gd < 1 || gd > 31) return false;

    int32_t jdn = g2d(gy, gm, gd);
    int32_t y, m, d;
    d2g(jdn, &y, &m, &d);                 /* also gives the Gregorian year of jdn */
    int32_t jy = y - 621;
    jalcal_t r;
    if (!jalcal(jy, &r)) return false;

    int32_t k = jdn - g2d(y, 3, r.march);
    int32_t jm, jd;
    if (k >= 0) {
        if (k <= 185) {
            jm = 1 + idiv(k, 31);
            jd = imod(k, 31) + 1;
            out->y = (uint16_t)jy; out->m = (uint8_t)jm; out->d = (uint8_t)jd;
            return true;
        }
        k -= 186;
    } else {
        jy -= 1;
        k += 179;
        if (r.leap == 1) k += 1;
    }
    jm = 7 + idiv(k, 30);
    jd = imod(k, 30) + 1;
    out->y = (uint16_t)jy; out->m = (uint8_t)jm; out->d = (uint8_t)jd;
    return true;
}

const char *jalali_month_name(uint8_t jm)
{
    static const char *const n[12] = {
        "Farvardin", "Ordibehesht", "Khordad", "Tir", "Mordad", "Shahrivar",
        "Mehr", "Aban", "Azar", "Dey", "Bahman", "Esfand"
    };
    return (jm >= 1 && jm <= 12) ? n[jm - 1] : "?";
}
