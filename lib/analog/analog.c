#include "user_config.h"
#include "analog.h"

/* soil probes: channel numbers 0..7 (PF0..PF7) */
#define ANALOG_PIN_MASK ((uint8_t)((1u << SOIL1_PIN) | (1u << SOIL2_PIN)))

void analog_init(void)
{
    DIDR0 = ANALOG_PIN_MASK;                         /* digital input buffers off */
    ADMUX = ANALOG_REFSEL;
    ADCSRB = 0;
    ADCSRA = _BV(ADEN) | _BV(ADPS2) | _BV(ADPS1) | _BV(ADPS0);   /* clk/128 = 125 kHz */
}

static uint16_t adc_once(void)
{
    ADCSRA |= _BV(ADSC);
    while (ADCSRA & _BV(ADSC)) { }
    return ADC;
}

uint16_t analog_read(uint8_t pin)
{
    uint32_t sum = 0;

    ADMUX = (uint8_t)(ANALOG_REFSEL | (pin & 0x07));
    (void)adc_once();                                /* dummy: let the mux settle */
    for (uint8_t i = 0; i < 16; i++) sum += adc_once();
    return (uint16_t)(sum >> 4);
}

uint8_t analog_to_percent(uint16_t raw, uint16_t raw_at_0, uint16_t raw_at_100)
{
    int32_t a = raw_at_0, b = raw_at_100;
    if (a == b) return 0;
    int32_t p = ((a - (int32_t)raw) * 100) / (a - b);
    if (p < 0) p = 0;
    if (p > 100) p = 100;
    return (uint8_t)p;
}
