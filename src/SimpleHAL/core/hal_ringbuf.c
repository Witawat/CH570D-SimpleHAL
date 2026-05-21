#include "hal_ringbuf.h"

/*********************************************************************
 * @fn      hal_ringbuf_init
 *
 * @brief   เริ่มต้น ring buffer: ผูก buffer และรีเซ็ต head/tail
 *
 * @param   rb      ตัวชี้ ring buffer
 * @param   buf     buffer สำหรับเก็บข้อมูล
 * @param   size    ขนาด buffer (ไบต์)
 *
 * @return  none
 */
void hal_ringbuf_init(hal_ringbuf_t *rb, uint8_t *buf, uint16_t size)
{
    rb->buf  = buf;
    rb->size = size;
    rb->head = 0;
    rb->tail = 0;
}

/*********************************************************************
 * @fn      hal_ringbuf_put
 *
 * @brief   ใส่ข้อมูล 1 ไบต์ ถ้า buffer เต็มคืน HAL_BUSY
 *
 * @param   rb      ตัวชี้ ring buffer
 * @param   data    ข้อมูล 1 ไบต์
 *
 * @return  HAL_OK ถ้าสำเร็จ, HAL_BUSY ถ้าเต็ม
 */
hal_status_t hal_ringbuf_put(hal_ringbuf_t *rb, uint8_t data)
{
    uint16_t next = (rb->head + 1) % rb->size;
    if (next == rb->tail) {
        return HAL_BUSY;
    }
    rb->buf[rb->head] = data;
    rb->head = next;
    return HAL_OK;
}

/*********************************************************************
 * @fn      hal_ringbuf_get
 *
 * @brief   อ่านข้อมูล 1 ไบต์ ถ้า buffer ว่างคืน HAL_BUSY
 *
 * @param   rb      ตัวชี้ ring buffer
 * @param   data    ตัวชี้รับข้อมูล 1 ไบต์
 *
 * @return  HAL_OK ถ้าสำเร็จ, HAL_BUSY ถ้าว่าง
 */
hal_status_t hal_ringbuf_get(hal_ringbuf_t *rb, uint8_t *data)
{
    if (rb->head == rb->tail) {
        return HAL_BUSY;
    }
    *data = rb->buf[rb->tail];
    rb->tail = (rb->tail + 1) % rb->size;
    return HAL_OK;
}

/*********************************************************************
 * @fn      hal_ringbuf_available
 *
 * @brief   คืนจำนวนข้อมูลที่รออ่าน
 *
 * @param   rb      ตัวชี้ ring buffer
 *
 * @return  จำนวนไบต์ที่พร้อมอ่าน
 */
uint16_t hal_ringbuf_available(hal_ringbuf_t *rb)
{
    return (rb->head + rb->size - rb->tail) % rb->size;
}

/*********************************************************************
 * @fn      hal_ringbuf_free_space
 *
 * @brief   คืนพื้นที่ว่างสำหรับเขียนข้อมูล
 *
 * @param   rb      ตัวชี้ ring buffer
 *
 * @return  จำนวนไบต์ที่เขียนเพิ่มได้
 */
uint16_t hal_ringbuf_free_space(hal_ringbuf_t *rb)
{
    return (rb->size - hal_ringbuf_available(rb) - 1);
}

/*********************************************************************
 * @fn      hal_ringbuf_flush
 *
 * @brief   ล้างข้อมูลทั้งหมด: รีเซ็ต head/tail
 *
 * @param   rb      ตัวชี้ ring buffer
 *
 * @return  none
 */
void hal_ringbuf_flush(hal_ringbuf_t *rb)
{
    rb->head = 0;
    rb->tail = 0;
}

/*********************************************************************
 * @fn      hal_ringbuf_is_empty
 *
 * @brief   ตรวจสอบว่า ring buffer ว่าง (head == tail)
 *
 * @param   rb      ตัวชี้ ring buffer
 *
 * @return  1 ถ้าว่าง, 0 ถ้าไม่ว่าง
 */
uint8_t hal_ringbuf_is_empty(hal_ringbuf_t *rb)
{
    return (rb->head == rb->tail);
}

/*********************************************************************
 * @fn      hal_ringbuf_is_full
 *
 * @brief   ตรวจสอบว่า ring buffer เต็ม (ถัดจาก head ชน tail)
 *
 * @param   rb      ตัวชี้ ring buffer
 *
 * @return  1 ถ้าเต็ม, 0 ถ้าไม่เต็ม
 */
uint8_t hal_ringbuf_is_full(hal_ringbuf_t *rb)
{
    return (((rb->head + 1) % rb->size) == rb->tail);
}
