#ifndef BUTTONS_H
#define BUTTONS_H

#include <stdint.h>

#define KEY_MODE       0x01   /* short press, reported on RELEASE            */
#define KEY_UP         0x02   /* press + auto-repeat                         */
#define KEY_DOWN       0x04   /* press + auto-repeat                         */
#define KEY_MODE_LONG  0x08   /* MODE held for 1 s (reported once, while held) */

void    buttons_init(void);
uint8_t buttons_poll(void);           /* returns a KEY_* bit mask of new events */
uint8_t buttons_repeat_count(void);   /* auto-repeats so far (for acceleration) */

#endif
