#ifndef AIR_SENSOR_H
#define AIR_SENSOR_H

#include <stdbool.h>
#include <stdint.h>

/*
 * Air temperature / humidity sensor. The type is a runtime setting
 * (settings_t.air_sensor, menu > FEATURES > Air sensor):
 *
 *   AIR_DHT11  one-wire on DHT_PIN, 0..50 C, +-2 C, +-5 % RH
 *   AIR_DHT12  I2C 0x5C, -20..60 C, +-0.5 C, +-5 % RH
 *   AIR_AHT20  I2C 0x38, -40..85 C, +-0.3 C, +-2 % RH  (AHT21 works the same way)
 */
#define AIR_DHT11  0
#define AIR_DHT12  1
#define AIR_AHT20  2
#define AIR_COUNT  3

void air_init(void);                              /* prepare pins; (re)start the selected sensor */
/* temperature in TENTHS of a degree C (245 = 24.5 C); DHT11 only has whole degrees */
bool air_read(int16_t *temp_c10, uint8_t *hum_pct);  /* false on error / bad checksum */

#endif
