#include "user_config.h"
#include <avr/interrupt.h>
#include <util/atomic.h>
#include "flow_meter.h"
#include "gpio.h"

#if FLOW_INT_NUM < 4 || FLOW_INT_NUM > 7
#error "FLOW_INT_NUM must be 4..7 (PE4..PE7)"
#endif

#define FLOW_VECT_(n)  INT##n##_vect
#define FLOW_VECT(n)   FLOW_VECT_(n)

static volatile uint32_t pulses;

ISR(FLOW_VECT(FLOW_INT_NUM))
{
    pulses++;
}

void flow_init(void)
{
    PIN_INPUT(FLOW_PIN);
    PIN_HIGH(FLOW_PIN);                                        /* pull-up */
    /* rising edge: ISCn1:ISCn0 = 11 (INT4..7 are in EICRB) */
    EICRB |= (uint8_t)(3u << ((FLOW_INT_NUM - 4) * 2));
    EIFR   = (uint8_t)_BV(FLOW_INT_NUM);                       /* clear stale flag */
    EIMSK |= (uint8_t)_BV(FLOW_INT_NUM);
}

uint32_t flow_pulses(void)
{
    uint32_t v;
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) { v = pulses; }
    return v;
}
