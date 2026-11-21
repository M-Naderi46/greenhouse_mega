#ifndef LCD_I2C_H
#define LCD_I2C_H

#include <stdbool.h>
#include <stdint.h>

void lcd_init(void);
void lcd_clear(void);
void lcd_backlight(bool on);
void lcd_line(uint8_t row, const char *s);   /* prints one row, pads with spaces */

#endif
