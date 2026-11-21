#ifndef OUTPUTS_H
#define OUTPUTS_H

#include <stdbool.h>

typedef enum { OUT_FAN = 0, OUT_HEATER, OUT_LAMP, OUT_PUMP, OUT_COUNT } out_id_t;

void outputs_init(void);                   /* all outputs start OFF         */
void outputs_set(out_id_t id, bool on);
bool outputs_get(out_id_t id);

#endif
