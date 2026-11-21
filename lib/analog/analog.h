#ifndef ANALOG_H
#define ANALOG_H

#include <stdint.h>

void     analog_init(void);              /* call BEFORE interrupts are enabled */
uint16_t analog_read(uint8_t pin);       /* 12-bit, averaged over 16 samples   */

/* Linear map: raw_at_0 -> 0 %, raw_at_100 -> 100 %. Works for both polarities. */
uint8_t  analog_to_percent(uint16_t raw, uint16_t raw_at_0, uint16_t raw_at_100);

#endif
