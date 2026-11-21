#include "user_config.h"
#include "light_sensor.h"
#include "hal_twi.h"

#define BH_POWER_ON   0x01
#define BH_RESET      0x07
#define BH_CONT_H_RES 0x10          /* continuous, 1 lx resolution mode      */
#define BH_MT_HIGH(m) ((uint8_t)(0x40 | ((m) >> 5)))
#define BH_MT_LOW(m)  ((uint8_t)(0x60 | ((m) & 0x1F)))
#define BH_MTREG      31            /* minimum sensitivity = widest range    */

static bool started;

static bool cmd(uint8_t c)
{
    return twi_write(BH1750_I2C_ADDR, &c, 1, true);
}

static bool start(void)
{
    started = cmd(BH_POWER_ON) && cmd(BH_RESET) &&
              cmd(BH_MT_HIGH(BH_MTREG)) && cmd(BH_MT_LOW(BH_MTREG)) &&
              cmd(BH_CONT_H_RES);
    return started;
}

void light_init(void)
{
    (void)start();
}

bool light_read_lux(uint16_t *lux)
{
    uint8_t d[2];

    if (!started && !start()) return false;

    if (!twi_read(BH1750_I2C_ADDR, d, 2)) {
        started = false;                      /* re-initialise on next call */
        return false;
    }

    uint32_t raw = ((uint32_t)d[0] << 8) | d[1];
    /* lux = raw / 1.2 * (69 / MTreg) */
    uint32_t v = (raw * 690UL) / (12UL * BH_MTREG);
    *lux = v > 65535UL ? 65535U : (uint16_t)v;
    return true;
}
