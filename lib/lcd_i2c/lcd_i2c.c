#include "user_config.h"
#include <util/delay.h>
#include "lcd_i2c.h"
#include "hal_twi.h"

static uint8_t backlight = LCD_BL_BIT;

static void lcd_send(uint8_t val, uint8_t rs)
{
    uint8_t hi = (uint8_t)((val & 0xF0) | backlight | rs);
    uint8_t lo = (uint8_t)(((val << 4) & 0xF0) | backlight | rs);
    uint8_t b[4] = { (uint8_t)(hi | LCD_EN_BIT), hi,
                     (uint8_t)(lo | LCD_EN_BIT), lo };
    twi_write(LCD_I2C_ADDR, b, 4, true);
    _delay_us(50);
}

static void lcd_cmd(uint8_t c)
{
    lcd_send(c, 0);
    if (c <= 3) _delay_ms(2);        /* clear / home are slow */
}

void lcd_init(void)
{
    uint8_t b[2];
    _delay_ms(50);
    for (uint8_t i = 0; i < 3; i++) {            /* force 8-bit mode */
        b[0] = (uint8_t)(0x30 | backlight | LCD_EN_BIT);
        b[1] = (uint8_t)(0x30 | backlight);
        twi_write(LCD_I2C_ADDR, b, 2, true);
        _delay_ms(5);
    }
    b[0] = (uint8_t)(0x20 | backlight | LCD_EN_BIT);   /* switch to 4-bit */
    b[1] = (uint8_t)(0x20 | backlight);
    twi_write(LCD_I2C_ADDR, b, 2, true);
    _delay_ms(5);

    lcd_cmd(0x28);   /* 4-bit, 2-line mode (also used by 4-line panels) */
    lcd_cmd(0x0C);   /* display on, cursor off */
    lcd_cmd(0x06);   /* entry mode: increment */
    lcd_cmd(0x01);   /* clear */
}

void lcd_clear(void)
{
    lcd_cmd(0x01);
}

void lcd_backlight(bool on)
{
    backlight = on ? LCD_BL_BIT : 0;
    twi_write(LCD_I2C_ADDR, &backlight, 1, true);
}

void lcd_line(uint8_t row, const char *s)
{
    static const uint8_t off[4] = { 0x00, 0x40,
                                    (uint8_t)LCD_COLS, (uint8_t)(0x40 + LCD_COLS) };
    if (row >= LCD_ROWS) return;

    uint8_t i = 0;
    lcd_cmd((uint8_t)(0x80 | off[row]));
    for (; i < LCD_COLS && s[i]; i++) lcd_send((uint8_t)s[i], LCD_RS_BIT);
    for (; i < LCD_COLS; i++)         lcd_send(' ', LCD_RS_BIT);
}
