#include "hal_gpio.h"
#include "simple_hal_config.h"
#include "CH57x_common.h"

static uint32_t pin_to_mask(uint8_t pin)
{
    return (1UL << pin);
}

struct hal_gpio_obj {
    uint8_t        pin;
    uint8_t        used;
    hal_callback_t cb;
    void          *arg;
};

static struct hal_gpio_obj gpio_objs[HAL_GPIO_MAX_PINS];

hal_gpio_handle_t hal_gpio_init(uint8_t pin, hal_gpio_mode_t mode)
{
    static GPIOModeTypeDef mode_map[] = {
        GPIO_ModeIN_Floating,
        GPIO_ModeIN_PU,
        GPIO_ModeIN_PD,
        GPIO_ModeOut_PP_5mA,
        GPIO_ModeOut_PP_20mA,
    };

    if (pin >= HAL_GPIO_MAX_PINS) return NULL;
    if (mode > HAL_GPIO_OUTPUT_PP_20mA) return NULL;

    if (gpio_objs[pin].used) return &gpio_objs[pin];

    gpio_objs[pin].pin  = pin;
    gpio_objs[pin].used = 1;
    gpio_objs[pin].cb   = NULL;
    gpio_objs[pin].arg  = NULL;

    GPIOA_ModeCfg(pin_to_mask(pin), mode_map[mode]);
    return &gpio_objs[pin];
}

void hal_gpio_write(hal_gpio_handle_t h, uint8_t val)
{
    if (!h || !h->used) return;
    if (val)
        GPIOA_SetBits(pin_to_mask(h->pin));
    else
        GPIOA_ResetBits(pin_to_mask(h->pin));
}

uint8_t hal_gpio_read(hal_gpio_handle_t h)
{
    if (!h || !h->used) return 0;
    return (GPIOA_ReadPortPin(pin_to_mask(h->pin)) != 0);
}

void hal_gpio_toggle(hal_gpio_handle_t h)
{
    if (!h || !h->used) return;
    GPIOA_InverseBits(pin_to_mask(h->pin));
}

void hal_gpio_set(hal_gpio_handle_t h)
{
    if (!h || !h->used) return;
    GPIOA_SetBits(pin_to_mask(h->pin));
}

void hal_gpio_reset(hal_gpio_handle_t h)
{
    if (!h || !h->used) return;
    GPIOA_ResetBits(pin_to_mask(h->pin));
}

hal_status_t hal_gpio_attach_irq(hal_gpio_handle_t h, hal_gpio_irq_mode_t mode, hal_callback_t cb, void *arg)
{
    static GPIOITModeTpDef irq_map[] = {
        GPIO_ITMode_LowLevel,
        GPIO_ITMode_HighLevel,
        GPIO_ITMode_FallEdge,
        GPIO_ITMode_RiseEdge,
    };

    if (!h || !h->used) return HAL_INVALID;
    if (!cb) return HAL_INVALID;

    h->cb  = cb;
    h->arg = arg;

    GPIOA_ITModeCfg(pin_to_mask(h->pin), irq_map[mode]);
    PFIC_EnableIRQ(GPIO_A_IRQn);
    return HAL_OK;
}

void hal_gpio_detach_irq(hal_gpio_handle_t h)
{
    if (!h || !h->used) return;
    h->cb  = NULL;
    h->arg = NULL;
}

void hal_gpio_int_handler(void)
{
    uint16_t flags = GPIOA_ReadITFlagPort();
    for (int i = 0; i < HAL_GPIO_MAX_PINS; i++) {
        if (flags & pin_to_mask(i)) {
            if (gpio_objs[i].used && gpio_objs[i].cb) {
                gpio_objs[i].cb(gpio_objs[i].arg);
            }
            GPIOA_ClearITFlagBit(pin_to_mask(i));
        }
    }
}

__INTERRUPT __HIGH_CODE void GPIOA_IRQHandler(void)
{
    hal_gpio_int_handler();
}
