#ifndef HAL_TWI_H
#define HAL_TWI_H

#include <stdint.h>
#include <stdbool.h>

void twi_init(void);

/* stop = false keeps the bus for a repeated start (use before twi_read) */
bool twi_write(uint8_t addr, const uint8_t *buf, uint8_t len, bool stop);
bool twi_read(uint8_t addr, uint8_t *buf, uint8_t len);

#endif
