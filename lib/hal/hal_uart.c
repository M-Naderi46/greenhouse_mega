#include "user_config.h"
#include <avr/interrupt.h>
#include <util/atomic.h>
#include "hal_uart.h"

/* USART0 (PE1 = TX0 = Arduino D1, also the USB serial port). TX only.
 * 256-byte ring buffer: uint8_t indices wrap by themselves.
 * One slot stays free to tell "full" from "empty" (255 bytes usable). */
static uint8_t buf[256];
static volatile uint8_t head, tail;

void uart_init(void)
{
    uint16_t ubrr = (uint16_t)((F_CPU + 8UL * UART_BAUD) / (16UL * UART_BAUD) - 1UL);
    UCSR0A = 0;
    UBRR0H = (uint8_t)(ubrr >> 8);
    UBRR0L = (uint8_t)(ubrr & 0xFF);
    UCSR0C = _BV(UCSZ01) | _BV(UCSZ00);          /* async, 8N1 */
    UCSR0B = _BV(TXEN0);
}

ISR(USART0_UDRE_vect)
{
    if (head == tail) {
        UCSR0B &= (uint8_t)~_BV(UDRIE0);         /* nothing left: stop the irq */
        return;
    }
    UDR0 = buf[tail];
    tail++;
}

bool uart_write(const char *data, uint8_t len)
{
    bool ok = false;
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        uint8_t used = (uint8_t)(head - tail);
        uint8_t room = (uint8_t)(255 - used);
        if (len <= room) {
            for (uint8_t i = 0; i < len; i++) buf[head++] = (uint8_t)data[i];
            UCSR0B |= _BV(UDRIE0);
            ok = true;
        }
    }
    return ok;
}
