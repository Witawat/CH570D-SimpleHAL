/*********************************************************************
 * @file    hal_cmp.h
 * @brief   SimpleHAL — โมดูล Comparator (CMP)
 *
 * เปรียบเทียบแรงดันระหว่างขา (+) และ (-) ให้ผลลัพธ์ดิจิตอล 0/1
 * รองรับ VREF ภายใน 50-800mV, interrupt, และ Timer capture
 */

#ifndef __HAL_CMP_H__
#define __HAL_CMP_H__

#include <stdint.h>
#include "core/hal_types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define HAL_CMP_MAX_INSTANCES 1

/**
 * @brief   การเลือกขาเข้า (+) และ (-) ของ Comparator
 */
typedef enum {
    HAL_CMP_INPUT_PA3_PA2   = 0,  /* (+)PA3, (-)PA2 */
    HAL_CMP_INPUT_PA3_VREF  = 1,  /* (+)PA3, (-)VREF ภายใน */
    HAL_CMP_INPUT_PA7_PA2   = 2,  /* (+)PA7, (-)PA2 */
    HAL_CMP_INPUT_PA7_VREF  = 3,  /* (+)PA7, (-)VREF ภายใน */
} hal_cmp_input_t;

/**
 * @brief   ระดับแรงดัน VREF ภายใน (50-800mV เพิ่มครั้งละ 50mV)
 */
typedef enum {
    HAL_CMP_VREF_50MV   = 0,   /*  50mV */
    HAL_CMP_VREF_100MV  = 1,   /* 100mV */
    HAL_CMP_VREF_150MV  = 2,   /* 150mV */
    HAL_CMP_VREF_200MV  = 3,   /* 200mV */
    HAL_CMP_VREF_250MV  = 4,   /* 250mV */
    HAL_CMP_VREF_300MV  = 5,   /* 300mV */
    HAL_CMP_VREF_350MV  = 6,   /* 350mV */
    HAL_CMP_VREF_400MV  = 7,   /* 400mV */
    HAL_CMP_VREF_450MV  = 8,   /* 450mV */
    HAL_CMP_VREF_500MV  = 9,   /* 500mV */
    HAL_CMP_VREF_550MV  = 10,  /* 550mV */
    HAL_CMP_VREF_600MV  = 11,  /* 600mV */
    HAL_CMP_VREF_650MV  = 12,  /* 650mV */
    HAL_CMP_VREF_700MV  = 13,  /* 700mV */
    HAL_CMP_VREF_750MV  = 14,  /* 750mV */
    HAL_CMP_VREF_800MV  = 15,  /* 800mV */
} hal_cmp_vref_t;

/**
 * @brief   เงื่อนไขการเกิดขัดจังหวะของ Comparator
 */
typedef enum {
    HAL_CMP_IRQ_HIGH_LEVEL = 0,  /* ขัดจังหวะเมื่อ output เป็น 1 */
    HAL_CMP_IRQ_LOW_LEVEL  = 1,  /* ขัดจังหวะเมื่อ output เป็น 0 */
    HAL_CMP_IRQ_FALLING    = 2,  /* ขัดจังหวะเมื่อ output ตกขอบ 1→0 */
    HAL_CMP_IRQ_RISING     = 3,  /* ขัดจังหวะเมื่อ output ขึ้นขอบ 0→1 */
} hal_cmp_irq_t;

typedef struct hal_cmp_obj *hal_cmp_handle_t;

/**
 * @brief   เริ่มต้น Comparator
 *
 * @param   input - การเลือกขา (+)/(-)
 * @param   vref  - ระดับ VREF (ใช้เมื่อเลือก VREF เป็น (-))
 *
 * @return  handle ของ CMP หรือ NULL หากมีข้อผิดพลาด
 */
hal_cmp_handle_t hal_cmp_init(hal_cmp_input_t input, hal_cmp_vref_t vref);

/**
 * @brief   ปิด Comparator และล้างสถานะ
 *
 * @param   h - handle ของ CMP
 */
void hal_cmp_deinit(hal_cmp_handle_t h);

/**
 * @brief   อ่านค่า output ปัจจุบันของ Comparator
 *
 * @param   h - handle ของ CMP
 *
 * @return  1 ถ้าแรงดัน (+) > (-), 0 ถ้า (+) < (-)
 */
uint8_t hal_cmp_read(hal_cmp_handle_t h);

/**
 * @brief   เปิดการทำงานของ Comparator
 *
 * @param   h - handle ของ CMP
 */
void hal_cmp_enable(hal_cmp_handle_t h);

/**
 * @brief   ปิดการทำงานของ Comparator
 *
 * @param   h - handle ของ CMP
 */
void hal_cmp_disable(hal_cmp_handle_t h);

/**
 * @brief   ส่งสัญญาณ output ของ CMP ไปยัง Timer capture input
 *
 * @param   h  - handle ของ CMP
 * @param   en - 1 = เชื่อมต่อ, 0 = ตัดการเชื่อมต่อ
 */
void hal_cmp_route_to_timer_cap(hal_cmp_handle_t h, uint8_t en);

/**
 * @brief   ผูก callback สำหรับขัดจังหวะของ Comparator
 *
 * @param   h       - handle ของ CMP
 * @param   trigger - เงื่อนไขการเกิดขัดจังหวะ
 * @param   cb      - ฟังก์ชัน callback
 * @param   arg     - อาร์กิวเมนต์ที่ส่งให้ callback
 */
void hal_cmp_attach_irq(hal_cmp_handle_t h, hal_cmp_irq_t trigger,
                        hal_callback_t cb, void *arg);

/**
 * @brief   ตัวจัดการขัดจังหวะ CMP หลัก เรียกจาก ISR
 */
void hal_cmp_int_handler(void);

#ifdef __cplusplus
}
#endif

#endif
