#include "user_config.h"
#include <avr/interrupt.h>
#include <util/atomic.h>
#include "hal_timebase.h"

static volatile uint32_t ms_ticks;

ISR(TIMER0_COMPA_vect)
{
    ms_ticks++;
}

/* Timer0, CTC mode, clk/64: 16 MHz / 64 / 250 = 1 kHz */
void timebase_init(void)
{
    TCCR0A = _BV(WGM01);
    OCR0A  = (uint8_t)(F_CPU / 64UL / 1000UL - 1UL);
    TCNT0  = 0;
    TIMSK0 = _BV(OCIE0A);
    TCCR0B = _BV(CS01) | _BV(CS00);
}

uint32_t millis(void)
{
    uint32_t v;
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) { v = ms_ticks; }
    return v;
}
