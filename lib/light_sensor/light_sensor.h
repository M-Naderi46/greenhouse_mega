#ifndef LIGHT_SENSOR_H
#define LIGHT_SENSOR_H

#include <stdbool.h>
#include <stdint.h>

/*
 * BH1750 digital light sensor (GY-302 module), I2C.
 *
 *   - result directly in lux (no calibration needed)
 *   - configured for the LOW sensitivity range (MTreg = 31): up to ~65000 lx,
 *     so even full summer sun does not saturate it; resolution ~2 lx
 *   - the sensor is (re)initialised automatically, so a module that is
 *     connected later or after an I2C glitch is picked up again
 */
void light_init(void);                     /* tries to start the sensor (ok if absent) */
bool light_read_lux(uint16_t *lux);        /* false if the sensor does not answer      */

#endif
