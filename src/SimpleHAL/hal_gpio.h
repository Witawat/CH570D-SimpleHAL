#ifndef __HAL_GPIO_H__
#define __HAL_GPIO_H__

#include <stdint.h>
#include "core/hal_types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define HAL_GPIO_MAX_PINS  16

#define PA0   0
#define PA1   1
#define PA2   2
#define PA3   3
#define PA4   4
#define PA5   5
#define PA6   6
#define PA7   7
#define PA8   8
#define PA9   9
#define PA10  10
#define PA11  11
#define PA12  12
#define PA13  13
#define PA14  14
#define PA15  15

/**
 * @brief   โหมดการทำงานของ GPIO
 */
typedef enum {
    HAL_GPIO_INPUT_FLOATING = 0,  /* อินพุตลอย */
    HAL_GPIO_INPUT_PULLUP,        /* อินพุตพุลอัป */
    HAL_GPIO_INPUT_PULLDOWN,      /* อินพุตพุลดาวน์ */
    HAL_GPIO_OUTPUT_PP_5mA,       /* เอาต์พุตพุชพุล 5mA */
    HAL_GPIO_OUTPUT_PP_20mA,      /* เอาต์พุตพุชพุล 20mA */
} hal_gpio_mode_t;

/**
 * @brief   โหมดขัดจังหวะ GPIO
 */
typedef enum {
    HAL_GPIO_IRQ_LOW_LEVEL  = 0,  /* ขัดจังหวะเมื่อสัญญาณต่ำ */
    HAL_GPIO_IRQ_HIGH_LEVEL,      /* ขัดจังหวะเมื่อสัญญาณสูง */
    HAL_GPIO_IRQ_FALLING,         /* ขัดจังหวะเมื่อสัญญาณตกขอบ */
    HAL_GPIO_IRQ_RISING,          /* ขัดจังหวะเมื่อสัญญาณขึ้นขอบ */
} hal_gpio_irq_mode_t;

typedef struct hal_gpio_obj *hal_gpio_handle_t;

/**
 * @brief   เริ่มต้น GPIO: กำหนดขาและโหมดการทำงาน
 *
 * @param   pin - หมายเลขขาที่ต้องการเริ่มต้น
 * @param   mode - โหมดการทำงานของ GPIO
 *
 * @return  handle ของ GPIO ที่เริ่มต้นแล้ว หรือ NULL หากผิดพลาด
 */
hal_gpio_handle_t hal_gpio_init(uint8_t pin, hal_gpio_mode_t mode);
/**
 * @brief   เขียนค่าระดับลอจิกให้ขา GPIO
 *
 * @param   h - handle ของ GPIO
 * @param   val - ค่าลอจิกที่ต้องการเขียน (0 หรือ 1)
 *
 * @return  ไม่มี
 */
void              hal_gpio_write(hal_gpio_handle_t h, uint8_t val);
/**
 * @brief   อ่านค่าระดับลอจิกจากขา GPIO
 *
 * @param   h - handle ของ GPIO
 *
 * @return  ค่าลอจิกปัจจุบัน (0 หรือ 1)
 */
uint8_t           hal_gpio_read(hal_gpio_handle_t h);
/**
 * @brief   กลับค่าระดับลอจิกของขา GPIO (toggle)
 *
 * @param   h - handle ของ GPIO
 *
 * @return  ไม่มี
 */
void              hal_gpio_toggle(hal_gpio_handle_t h);
/**
 * @brief   เซ็ตขา GPIO เป็นระดับสูง (1)
 *
 * @param   h - handle ของ GPIO
 *
 * @return  ไม่มี
 */
void              hal_gpio_set(hal_gpio_handle_t h);
/**
 * @brief   รีเซ็ตขา GPIO เป็นระดับต่ำ (0)
 *
 * @param   h - handle ของ GPIO
 *
 * @return  ไม่มี
 */
void              hal_gpio_reset(hal_gpio_handle_t h);
/**
 * @brief   ผูกฟังก์ชัน callback สำหรับขัดจังหวะ GPIO
 *
 * @param   h - handle ของ GPIO
 * @param   mode - โหมดขัดจังหวะ
 * @param   cb - ฟังก์ชัน callback ที่จะเรียกเมื่อเกิดขัดจังหวะ
 * @param   arg - อาร์กิวเมนต์ที่ส่งให้ callback
 *
 * @return  HAL_OK หากสำเร็จ, HAL_INVALID หากพารามิเตอร์ไม่ถูกต้อง
 */
hal_status_t      hal_gpio_attach_irq(hal_gpio_handle_t h, hal_gpio_irq_mode_t mode, hal_callback_t cb, void *arg);
/**
 * @brief   ยกเลิกการผูก callback ขัดจังหวะ GPIO
 *
 * @param   h - handle ของ GPIO
 *
 * @return  ไม่มี
 */
void              hal_gpio_detach_irq(hal_gpio_handle_t h);
/**
 * @brief   ตัวจัดการขัดจังหวะ GPIO หลัก เรียกจาก ISR
 *
 * @return  ไม่มี
 */
void              hal_gpio_int_handler(void);

#ifdef __cplusplus
}
#endif

#endif
