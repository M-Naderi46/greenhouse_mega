#ifndef SOIL_SENSOR_H
#define SOIL_SENSOR_H

#include <stdbool.h>
#include <stdint.h>

void soil_init(void);                    /* configures the optional power switch pin */

/* Reads 'count' probes (1 or 2); probe power is switched on once if enabled.
 * raw[1] is 0 when count == 1.                                            */
void soil_read_all(uint16_t raw[2], uint8_t count);

bool soil_raw_valid(uint16_t raw);       /* false if probe open/short */

#endif
