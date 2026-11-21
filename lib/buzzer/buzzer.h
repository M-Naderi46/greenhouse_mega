#ifndef BUZZER_H
#define BUZZER_H

#include <stdbool.h>

void buzzer_init(void);
void buzzer_task(bool alarm_on);   /* call often; blinks/beeps at 1 Hz while active */

#endif
