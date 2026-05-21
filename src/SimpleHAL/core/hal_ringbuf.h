#ifndef __HAL_RINGBUF_H__
#define __HAL_RINGBUF_H__

#include <stdint.h>
#include "hal_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief   โครงสร้าง ring buffer
 */
typedef struct {
    uint8_t *buf;    /**< พอยน์เตอร์ไปยัง buffer จริง */
    uint16_t size;   /**< ขนาด buffer (ไบต์) */
    uint16_t head;   /**< ตำแหน่งเขียนถัดไป */
    uint16_t tail;   /**< ตำแหน่งอ่านถัดไป */
} hal_ringbuf_t;

/**
 * @brief   เริ่มต้น ring buffer
 * @param   rb      ตัวชี้ไปยังโครงสร้าง ring buffer
 * @param   buf     buffer จริงที่ใช้เก็บข้อมูล
 * @param   size    ขนาดของ buffer (ไบต์)
 */
void     hal_ringbuf_init(hal_ringbuf_t *rb, uint8_t *buf, uint16_t size);

/**
 * @brief   ใส่ข้อมูล 1 ไบต์
 * @param   rb      ตัวชี้ ring buffer
 * @param   data    ข้อมูล 1 ไบต์ที่ต้องการใส่
 * @return  HAL_OK ถ้าสำเร็จ, HAL_BUSY ถ้าเต็ม
 */
hal_status_t hal_ringbuf_put(hal_ringbuf_t *rb, uint8_t data);

/**
 * @brief   อ่านข้อมูล 1 ไบต์
 * @param   rb      ตัวชี้ ring buffer
 * @param   data    ตัวชี้ไปยังตัวแปรรับข้อมูล
 * @return  HAL_OK ถ้าสำเร็จ, HAL_BUSY ถ้าว่าง
 */
hal_status_t hal_ringbuf_get(hal_ringbuf_t *rb, uint8_t *data);

/**
 * @brief   จำนวนข้อมูลที่พร้อมอ่าน
 * @param   rb      ตัวชี้ ring buffer
 * @return  จำนวนไบต์ที่รออ่าน
 */
uint16_t hal_ringbuf_available(hal_ringbuf_t *rb);

/**
 * @brief   พื้นที่ว่างที่เขียนได้
 * @param   rb      ตัวชี้ ring buffer
 * @return  จำนวนไบต์ที่เขียนเพิ่มได้
 */
uint16_t hal_ringbuf_free_space(hal_ringbuf_t *rb);

/**
 * @brief   ล้างข้อมูลทั้งหมดใน ring buffer
 * @param   rb      ตัวชี้ ring buffer
 */
void     hal_ringbuf_flush(hal_ringbuf_t *rb);

/**
 * @brief   ตรวจสอบว่า ring buffer ว่างหรือไม่
 * @param   rb      ตัวชี้ ring buffer
 * @return  1 ถ้าว่าง, 0 ถ้าไม่ว่าง
 */
uint8_t  hal_ringbuf_is_empty(hal_ringbuf_t *rb);

/**
 * @brief   ตรวจสอบว่า ring buffer เต็มหรือไม่
 * @param   rb      ตัวชี้ ring buffer
 * @return  1 ถ้าเต็ม, 0 ถ้าไม่เต็ม
 */
uint8_t  hal_ringbuf_is_full(hal_ringbuf_t *rb);

#ifdef __cplusplus
}
#endif

#endif
