#ifndef TANK_H
#define TANK_H

#include <stdbool.h>

void tank_init(void);
void tank_update(void);      /* call once per second (debounce counter)        */
bool tank_empty(void);       /* raw debounced state; the FEAT_TANK switch is
                                applied in sensors.c                           */

#endif
