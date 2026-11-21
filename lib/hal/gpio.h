#ifndef GPIO_H
#define GPIO_H

/*
 * Pin helpers. A pin is written "<port letter>, <bit>" in user_config.h:
 *
 *     #define VALVE_PIN  A, 0
 *     PIN_OUTPUT(VALVE_PIN);  PIN_SET(1, VALVE_PIN);
 *
 * The extra macro layer (__VA_ARGS__) makes the preprocessor expand
 * VALVE_PIN into "A, 0" before the port letter is pasted into PORTA / DDRA.
 */
#include <avr/io.h>
#include <stdint.h>
#include <stdbool.h>

#define GPIO_DDR_(p, b)   DDR##p
#define GPIO_PRT_(p, b)   PORT##p
#define GPIO_PIN_(p, b)   PIN##p
#define GPIO_BM_(p, b)    ((uint8_t)(1u << (b)))

#define PIN_DDR(...)      GPIO_DDR_(__VA_ARGS__)
#define PIN_PRT(...)      GPIO_PRT_(__VA_ARGS__)
#define PIN_IN(...)       GPIO_PIN_(__VA_ARGS__)
#define PIN_BM(...)       GPIO_BM_(__VA_ARGS__)

#define PIN_OUTPUT(...)   (PIN_DDR(__VA_ARGS__) |=  PIN_BM(__VA_ARGS__))
#define PIN_INPUT(...)    (PIN_DDR(__VA_ARGS__) &= (uint8_t)~PIN_BM(__VA_ARGS__))
#define PIN_HIGH(...)     (PIN_PRT(__VA_ARGS__) |=  PIN_BM(__VA_ARGS__))   /* output high, or pull-up if input */
#define PIN_LOW(...)      (PIN_PRT(__VA_ARGS__) &= (uint8_t)~PIN_BM(__VA_ARGS__))
#define PIN_TOGGLE(...)   (PIN_IN(__VA_ARGS__) = PIN_BM(__VA_ARGS__))     /* writing 1 to PINx toggles */
#define PIN_READ(...)     ((PIN_IN(__VA_ARGS__) & PIN_BM(__VA_ARGS__)) != 0)

/* PIN_SET(level, PIN):  drive the pin high (level != 0) or low */
#define PIN_SET(level, ...) \
    do { if (level) PIN_HIGH(__VA_ARGS__); else PIN_LOW(__VA_ARGS__); } while (0)

/* registers by address (for tables) */
#define PIN_PRT_ADDR(...) (&PIN_PRT(__VA_ARGS__))
#define PIN_DDR_ADDR(...) (&PIN_DDR(__VA_ARGS__))

#endif
