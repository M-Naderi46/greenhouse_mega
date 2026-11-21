#ifndef HAL_TIMEBASE_H
#define HAL_TIMEBASE_H

#include <stdint.h>

void     timebase_init(void);   /* 1 kHz tick on Timer0 */
uint32_t millis(void);

#endif
