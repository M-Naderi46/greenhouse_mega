#include "air_decode.h"

uint8_t aht20_crc8(const uint8_t *data, uint8_t len)
{
    uint8_t crc = 0xFF;
    while (len--) {
        crc ^= *data++;
        for (uint8_t i = 0; i < 8; i++)
            crc = (crc & 0x80) ? (uint8_t)((crc << 1) ^ 0x31) : (uint8_t)(crc << 1);
    }
    return crc;
}

bool aht20_decode(const uint8_t d[7], int16_t *temp_c10, uint8_t *hum_pct)
{
    if (d[0] & 0x80) return false;                 /* measurement still running */
    if (!(d[0] & 0x08)) return false;              /* sensor not calibrated     */
    if (aht20_crc8(d, 6) != d[6]) return false;

    uint32_t hraw = ((uint32_t)d[1] << 12) | ((uint32_t)d[2] << 4) | (d[3] >> 4);
    uint32_t traw = ((uint32_t)(d[3] & 0x0F) << 16) | ((uint32_t)d[4] << 8) | d[5];

    /* RH % = raw / 2^20 * 100, rounded to the nearest whole percent */
    uint32_t rh = (hraw * 100UL + (1UL << 19)) >> 20;
    if (rh > 100) rh = 100;

    /* deg C = raw / 2^20 * 200 - 50. Work with n = deg C * 2^20 (exact integer),
     * multiply by 10 for tenths of a degree and round ONCE, half away from zero
     * (rounding twice gives wrong values for negative temperatures near x.x5).
     * |n * 10| <= 1.6e9, which fits in int32_t. */
    int32_t n  = ((int32_t)(traw * 200UL) - (50L << 20)) * 10L;
    int32_t t10 = (n >= 0) ? (int32_t)(((uint32_t)n + (1UL << 19)) >> 20)
                           : -(int32_t)(((uint32_t)(-n) + (1UL << 19)) >> 20);
    if (t10 < -450 || t10 > 900) return false;              /* outside the sensor range */

    *temp_c10 = (int16_t)t10;
    *hum_pct = (uint8_t)rh;
    return true;
}
