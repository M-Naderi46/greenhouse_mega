#ifndef IRRIGATION_H
#define IRRIGATION_H

#include <stdbool.h>
#include <stdint.h>

typedef enum {
    IRR_FAULT_NONE = 0,
    IRR_FAULT_NO_FLOW,        /* valve open but no water flowing */
    IRR_FAULT_LEAK            /* water flowing while valve closed */
} irr_fault_t;

void        irrigation_init(void);
void        irrigation_update(void);        /* call once per second */

bool        irrigation_is_open(void);
irr_fault_t irrigation_fault(void);
void        irrigation_clear_fault(void);
void        irrigation_manual(bool on);     /* valve test (from the menu) */

uint32_t    irrigation_lpm10(void);             /* flow, litres/min x 10 */
uint32_t    irrigation_cycle_liters10(void);    /* litres this cycle x 10 */
uint32_t    irrigation_day_liters10(void);      /* litres today x 10 */

#endif
