#ifndef FLOW_METER_H
#define FLOW_METER_H

#include <stdint.h>

void     flow_init(void);       /* rising-edge interrupt counter */
uint32_t flow_pulses(void);     /* total pulses since boot (atomic read) */

#endif
