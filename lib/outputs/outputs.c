#include "user_config.h"
#include "outputs.h"
#include "gpio.h"

typedef struct {
    volatile uint8_t *port;
    volatile uint8_t *ddr;
    uint8_t bm;
    uint8_t active_high;
} out_cfg_t;

#define OUT_ENTRY(pin, act) \
    { PIN_PRT_ADDR(pin), PIN_DDR_ADDR(pin), PIN_BM(pin), (act) }

static const out_cfg_t cfg[OUT_COUNT] = {
    [OUT_FAN]    = OUT_ENTRY(FAN_PIN,    FAN_ACTIVE_HIGH),
    [OUT_HEATER] = OUT_ENTRY(HEATER_PIN, HEATER_ACTIVE_HIGH),
    [OUT_LAMP]   = OUT_ENTRY(LAMP_PIN,   LAMP_ACTIVE_HIGH),
    [OUT_PUMP]   = OUT_ENTRY(PUMP_PIN,   PUMP_ACTIVE_HIGH),
};

static bool state[OUT_COUNT];

void outputs_set(out_id_t id, bool on)
{
    if (id >= OUT_COUNT) return;
    bool level = cfg[id].active_high ? on : !on;
    if (level) *cfg[id].port |= cfg[id].bm;
    else       *cfg[id].port &= (uint8_t)~cfg[id].bm;
    state[id] = on;
}

void outputs_init(void)
{
    for (uint8_t i = 0; i < OUT_COUNT; i++) {
        outputs_set((out_id_t)i, false);          /* level first, then output */
        *cfg[i].ddr |= cfg[i].bm;
    }
}

bool outputs_get(out_id_t id)
{
    return id < OUT_COUNT ? state[id] : false;
}
