#ifndef SETTINGS_MENU_H
#define SETTINGS_MENU_H

#include <stdbool.h>
#include <stdint.h>

bool menu_active(void);
void menu_enter(void);
void menu_key(uint8_t keys);    /* KEY_* bit mask from buttons_poll() */
void menu_task(void);           /* idle timeout */
void menu_draw(void);

#endif
