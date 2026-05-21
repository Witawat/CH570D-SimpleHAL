#include "hal_gpio.h"
#include "simple_hal_config.h"
#include "CH57x_common.h"

/*********************************************************************
 * @fn      pin_to_mask
 *
 * @brief   แปลงหมายเลขขา (0-15) เป็น bitmask
 *
 * @param   pin - หมายเลขขา 0-15
 *
 * @return  bitmask ของขาที่ระบุ
 */
static uint32_t pin_to_mask(uint8_t pin)
{
    return (1UL << pin);
}

/* โครงสร้างข้อมูลภายในของแต่ละขา GPIO */
struct hal_gpio_obj {
    uint8_t        pin;   /* หมายเลขขา */
    uint8_t        used;  /*  flag บอกว่าขานี้ถูกใช้งานแล้ว */
    hal_callback_t cb;    /* callback เมื่อเกิดขัดจังหวะ */
    void          *arg;   /* อาร์กิวเมนต์ที่ส่งให้ callback */
};

static struct hal_gpio_obj gpio_objs[HAL_GPIO_MAX_PINS];

/*********************************************************************
 * @fn      hal_gpio_init
 *
 * @brief   เริ่มต้น GPIO: ตรวจสอบพารามิเตอร์ ผูก object และกำหนดโหมดขา
 *
 * @param   pin - หมายเลขขา
 * @param   mode - โหมดการทำงาน
 *
 * @return  handle ของ GPIO หรือ NULL หากผิดพลาด
 */
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

/*********************************************************************
 * @fn      hal_gpio_write
 *
 * @brief   เขียนค่าระดับลอจิก 0 หรือ 1 ให้ขา GPIO
 *
 * @param   h - handle ของ GPIO
 * @param   val - ค่าลอจิก (0 หรือ 1)
 *
 * @return  ไม่มี
 */
void hal_gpio_write(hal_gpio_handle_t h, uint8_t val)
{
    if (!h || !h->used) return;
    if (val)
        GPIOA_SetBits(pin_to_mask(h->pin));
    else
        GPIOA_ResetBits(pin_to_mask(h->pin));
}

/*********************************************************************
 * @fn      hal_gpio_read
 *
 * @brief   อ่านค่าระดับลอจิกปัจจุบันของขา GPIO
 *
 * @param   h - handle ของ GPIO
 *
 * @return  ค่าลอจิกปัจจุบัน (0 หรือ 1)
 */
uint8_t hal_gpio_read(hal_gpio_handle_t h)
{
    if (!h || !h->used) return 0;
    return (GPIOA_ReadPortPin(pin_to_mask(h->pin)) != 0);
}

/*********************************************************************
 * @fn      hal_gpio_toggle
 *
 * @brief   กลับค่าระดับลอจิก (0→1, 1→0)
 *
 * @param   h - handle ของ GPIO
 *
 * @return  ไม่มี
 */
void hal_gpio_toggle(hal_gpio_handle_t h)
{
    if (!h || !h->used) return;
    GPIOA_InverseBits(pin_to_mask(h->pin));
}

/*********************************************************************
 * @fn      hal_gpio_set
 *
 * @brief   เซ็ตขาเป็น 1
 *
 * @param   h - handle ของ GPIO
 *
 * @return  ไม่มี
 */
void hal_gpio_set(hal_gpio_handle_t h)
{
    if (!h || !h->used) return;
    GPIOA_SetBits(pin_to_mask(h->pin));
}

/*********************************************************************
 * @fn      hal_gpio_reset
 *
 * @brief   เซ็ตขาเป็น 0
 *
 * @param   h - handle ของ GPIO
 *
 * @return  ไม่มี
 */
void hal_gpio_reset(hal_gpio_handle_t h)
{
    if (!h || !h->used) return;
    GPIOA_ResetBits(pin_to_mask(h->pin));
}

/*********************************************************************
 * @fn      hal_gpio_attach_irq
 *
 * @brief   ผูก callback กับโหมดขัดจังหวะที่เลือก และเปิด IRQ
 *
 * @param   h - handle ของ GPIO
 * @param   mode - โหมดขัดจังหวะ
 * @param   cb - ฟังก์ชัน callback
 * @param   arg - อาร์กิวเมนต์ที่ส่งให้ callback
 *
 * @return  HAL_OK หากสำเร็จ, HAL_INVALID หากพารามิเตอร์ไม่ถูกต้อง
 */
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

/*********************************************************************
 * @fn      hal_gpio_detach_irq
 *
 * @brief   ยกเลิกการผูก callback ขัดจังหวะ
 *
 * @param   h - handle ของ GPIO
 *
 * @return  ไม่มี
 */
void hal_gpio_detach_irq(hal_gpio_handle_t h)
{
    if (!h || !h->used) return;
    h->cb  = NULL;
    h->arg = NULL;
}

/*********************************************************************
 * @fn      hal_gpio_int_handler
 *
 * @brief   ตัวจัดการขัดจังหวะ: วนอ่าน flag แต่ละขา ถ้ามี callback ก็เรียก
 *
 * @return  ไม่มี
 */
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

/*********************************************************************
 * @fn      GPIOA_IRQHandler
 *
 * @brief   ISR จริงของ GPIO: เรียก hal_gpio_int_handler
 *
 * @return  ไม่มี
 */
__INTERRUPT __HIGH_CODE void GPIOA_IRQHandler(void)
{
    hal_gpio_int_handler();
}
