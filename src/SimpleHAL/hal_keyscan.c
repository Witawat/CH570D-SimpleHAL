#include "hal_keyscan.h"
#include "CH57x_common.h"

struct hal_keyscan_obj {
    uint8_t        used;
    uint16_t       pins;
    hal_callback_t cb;
    void          *arg;
};

static struct hal_keyscan_obj keyscan_instances[HAL_KEYSCAN_MAX_INSTANCES];

static hal_keyscan_handle_t get_keyscan(void)
{
    return &keyscan_instances[0];
}

hal_keyscan_handle_t hal_keyscan_init(uint16_t pins, uint8_t clk_div, uint8_t repeat)
{
    hal_keyscan_handle_t h = get_keyscan();
    if (h->used) return h;
    h->used = 1;
    h->pins = pins;
    h->cb   = NULL;
    h->arg  = NULL;

    KeyScan_Cfg(ENABLE, pins, clk_div, repeat);
    return h;
}

uint32_t hal_keyscan_get_value(hal_keyscan_handle_t h)
{
    (void)h;
    return KeyValue;
}

uint32_t hal_keyscan_get_count(hal_keyscan_handle_t h)
{
    (void)h;
    return KeyScan_Cnt;
}

void hal_keyscan_attach_cb(hal_keyscan_handle_t h, hal_callback_t cb, void *arg)
{
    if (!h || !h->used) return;
    h->cb  = cb;
    h->arg = arg;

    KeyScan_ITCfg(ENABLE, RB_KEY_PRESSED_IE | RB_SCAN_1END_IE);
    PFIC_EnableIRQ(KEYSCAN_IRQn);
}

void hal_keyscan_enable_wakeup(hal_keyscan_handle_t h, uint8_t enable)
{
    (void)h;
    KeyPress_Wake(enable ? ENABLE : DISABLE);
}

__INTERRUPT __HIGH_CODE void KEYSCAN_IRQHandler(void)
{
    hal_keyscan_handle_t h = get_keyscan();
    if (!h || !h->used) return;

    uint8_t flags = KeyScan_GetITFlag(0xFF);
    KeyScan_ClearITFlag(flags);

    if (h->cb) {
        h->cb(h->arg);
    }
}
