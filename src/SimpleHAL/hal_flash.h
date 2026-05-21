#ifndef __HAL_FLASH_H__
#define __HAL_FLASH_H__

#include <stdint.h>
#include "core/hal_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief   อ่านข้อมูลจาก Flash ที่ addr ไปยัง buf
 *
 * @param   addr - ที่อยู่ Flash
 * @param   buf - บัฟเฟอร์สำหรับรับข้อมูล
 * @param   len - จำนวนไบต์ที่อ่าน
 *
 * @return  สถานะการทำงาน
 */
hal_status_t hal_flash_read(uint32_t addr, void *buf, uint32_t len);
/**
 * @brief   อ่าน Unique ID จำนวน 8 ไบต์ของชิพ
 *
 * @param   buf - บัฟเฟอร์ขนาด 8 ไบต์สำหรับรับ Unique ID
 *
 * @return  สถานะการทำงาน
 */
hal_status_t hal_flash_get_unique_id(uint8_t buf[8]);
/**
 * @brief   อ่านค่า config จาก Flash
 *
 * @param   config_val - ตัวชี้ไปยังค่าที่อ่านได้
 *
 * @return  สถานะการทำงาน
 */
hal_status_t hal_flash_read_config(uint16_t *config_val);

#ifdef __cplusplus
}
#endif

#endif
