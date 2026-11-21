#ifndef VALVE_H
#define VALVE_H

#include <stdbool.h>

void valve_init(void);          /* configures pin, valve starts CLOSED */
void valve_set(bool open);
bool valve_is_open(void);

#endif
