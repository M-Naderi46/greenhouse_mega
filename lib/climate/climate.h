#ifndef CLIMATE_H
#define CLIMATE_H

#include <stdbool.h>
#include "outputs.h"

void climate_init(void);
void climate_update(void);                 /* call once per second */

/* true if this output is enabled in menu > FEATURES
 * (the lamp additionally needs the light sensor feature) */
bool climate_enabled(out_id_t id);

/* manual override (used by the menu test items) */
void climate_force(out_id_t id, bool on);  /* output stays in this state ...   */
void climate_release(void);                /* ... until released: back to auto */

#endif
