#ifndef AIR_DECODE_H
#define AIR_DECODE_H

#include <stdbool.h>
#include <stdint.h>

/* Pure functions (no hardware access), so they can be unit-tested on a PC. */

/* CRC-8 used by the AHT20: init 0xFF, polynomial 0x31 */
uint8_t aht20_crc8(const uint8_t *data, uint8_t len);

/*
 * Decode the 7 bytes returned by the AHT20 after a measurement:
 *   d[0] status, d[1..5] 20-bit humidity + 20-bit temperature, d[6] CRC.
 * Returns false if the sensor is still busy, not calibrated, the CRC is wrong
 * or the value is outside the sensor's physical range.
 * Temperature is returned in tenths of a degree C (245 = 24.5 C), rounded to the
 * nearest 0.1; humidity is rounded to whole % RH.
 */
bool aht20_decode(const uint8_t d[7], int16_t *temp_c10, uint8_t *hum_pct);

#endif
