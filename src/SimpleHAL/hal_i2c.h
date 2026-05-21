#ifndef __HAL_I2C_H__
#define __HAL_I2C_H__

#include <stdint.h>
#include "core/hal_types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define HAL_I2C_MAX_INSTANCES 1

/**
 * @brief   บทบาท I2C
 */
typedef enum {
    HAL_I2C_MASTER = 0,
    HAL_I2C_SLAVE,
} hal_i2c_role_t;

typedef enum {
    HAL_I2C_SPEED_STANDARD = 100000,
    HAL_I2C_SPEED_FAST     = 400000,
} hal_i2c_speed_t;

typedef struct hal_i2c_obj *hal_i2c_handle_t;

/**
 * @brief   เริ่มต้น I2C: กำหนดความถี่, บทบาท และที่อยู่ตัวเอง
 *
 * @param   clock_hz - ความถี่ I2C
 * @param   role - บทบาท (MASTER/SLAVE)
 * @param   own_addr - ที่อยู่ตัวเอง (สำหรับ slave)
 *
 * @return  handle ของ I2C หรือ NULL หากล้มเหลว
 */
hal_i2c_handle_t hal_i2c_init(uint32_t clock_hz, hal_i2c_role_t role, uint16_t own_addr);
/**
 * @brief   เขียนข้อมูลไปยัง device ผ่าน register address
 *
 * @param   h - handle ของ I2C
 * @param   dev_addr - ที่อยู่ device
 * @param   reg - ที่อยู่ register
 * @param   data - ข้อมูลที่จะเขียน
 * @param   len - จำนวนไบต์
 *
 * @return  HAL_OK หากสำเร็จ, HAL_TIMEOUT หากหมดเวลา
 */
hal_status_t     hal_i2c_write(hal_i2c_handle_t h, uint8_t dev_addr, uint8_t reg, const uint8_t *data, uint16_t len);
/**
 * @brief   อ่านข้อมูลจาก device ผ่าน register address
 *
 * @param   h - handle ของ I2C
 * @param   dev_addr - ที่อยู่ device
 * @param   reg - ที่อยู่ register
 * @param   data - บัฟเฟอร์รับข้อมูล
 * @param   len - จำนวนไบต์
 *
 * @return  HAL_OK หากสำเร็จ, HAL_TIMEOUT หากหมดเวลา
 */
hal_status_t     hal_i2c_read(hal_i2c_handle_t h, uint8_t dev_addr, uint8_t reg, uint8_t *data, uint16_t len);
/**
 * @brief   เขียนข้อมูลดิบไปยัง device โดยไม่ระบุ register
 *
 * @param   h - handle ของ I2C
 * @param   dev_addr - ที่อยู่ device
 * @param   data - ข้อมูลที่จะเขียน
 * @param   len - จำนวนไบต์
 *
 * @return  HAL_OK หากสำเร็จ, HAL_TIMEOUT หากหมดเวลา
 */
hal_status_t     hal_i2c_write_raw(hal_i2c_handle_t h, uint8_t dev_addr, const uint8_t *data, uint16_t len);
/**
 * @brief   อ่านข้อมูลดิบจาก device โดยไม่ระบุ register
 *
 * @param   h - handle ของ I2C
 * @param   dev_addr - ที่อยู่ device
 * @param   data - บัฟเฟอร์รับข้อมูล
 * @param   len - จำนวนไบต์
 *
 * @return  HAL_OK หากสำเร็จ, HAL_TIMEOUT หากหมดเวลา
 */
hal_status_t     hal_i2c_read_raw(hal_i2c_handle_t h, uint8_t dev_addr, uint8_t *data, uint16_t len);
/**
 * @brief   เขียนข้อมูลไปยัง device ผ่าน memory address (16-bit)
 *
 * @param   h - handle ของ I2C
 * @param   dev_addr - ที่อยู่ device
 * @param   mem_addr - ที่อยู่หน่วยความจำ (16-bit)
 * @param   addr_len - ความยาวที่อยู่ (1 หรือ 2 ไบต์)
 * @param   data - ข้อมูลที่จะเขียน
 * @param   len - จำนวนไบต์
 *
 * @return  HAL_OK หากสำเร็จ, HAL_TIMEOUT หากหมดเวลา
 */
hal_status_t     hal_i2c_mem_write(hal_i2c_handle_t h, uint8_t dev_addr, uint16_t mem_addr, uint8_t addr_len, const uint8_t *data, uint16_t len);
/**
 * @brief   อ่านข้อมูลจาก device ผ่าน memory address (16-bit)
 *
 * @param   h - handle ของ I2C
 * @param   dev_addr - ที่อยู่ device
 * @param   mem_addr - ที่อยู่หน่วยความจำ (16-bit)
 * @param   addr_len - ความยาวที่อยู่ (1 หรือ 2 ไบต์)
 * @param   data - บัฟเฟอร์รับข้อมูล
 * @param   len - จำนวนไบต์
 *
 * @return  HAL_OK หากสำเร็จ, HAL_TIMEOUT หากหมดเวลา
 */
hal_status_t     hal_i2c_mem_read(hal_i2c_handle_t h, uint8_t dev_addr, uint16_t mem_addr, uint8_t addr_len, uint8_t *data, uint16_t len);
/**
 * @brief   เปลี่ยนความถี่ I2C
 *
 * @param   h - handle ของ I2C
 * @param   clock_hz - ความถี่ใหม่
 */
void             hal_i2c_set_speed(hal_i2c_handle_t h, uint32_t clock_hz);

#ifdef __cplusplus
}
#endif

#endif
