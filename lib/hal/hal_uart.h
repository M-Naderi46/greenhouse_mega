#ifndef HAL_UART_H
#define HAL_UART_H

#include <stdbool.h>
#include <stdint.h>

void uart_init(void);

/* Queue a block for transmission (interrupt driven, never blocks).
 * All-or-nothing: returns false and drops the block if the buffer is full. */
bool uart_write(const char *data, uint8_t len);

#endif
