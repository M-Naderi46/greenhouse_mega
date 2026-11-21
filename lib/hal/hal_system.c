#include "user_config.h"
#include <avr/interrupt.h>
#include <avr/wdt.h>
#include "hal_system.h"

/*
 * Runs very early (before main). After a watchdog reset the watchdog stays
 * enabled with a short timeout, so it must be switched off here, otherwise the
 * chip keeps resetting.
 */
void wdt_early_off(void) __attribute__((naked, used, section(".init3")));
void wdt_early_off(void)
{
    MCUSR = 0;
    wdt_disable();
}

void system_enable_interrupts(void)
{
    sei();
}

void system_wdt_start(void)
{
#if WDT_ENABLED
    wdt_enable(WDTO_4S);
#endif
}
