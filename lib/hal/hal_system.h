#ifndef HAL_SYSTEM_H
#define HAL_SYSTEM_H

void system_enable_interrupts(void);
void system_wdt_start(void);      /* ~4 s watchdog (user_config.h: WDT_ENABLED) */

#endif
