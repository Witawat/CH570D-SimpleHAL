#include "hal_flash.h"
#include "CH57x_common.h"

/*********************************************************************
 * @fn      hal_flash_read
 *
 * @brief   อ่านข้อมูลจาก Flash ROM
 *
 * @param   addr - ที่อยู่เริ่มต้นใน Flash
 * @param   buf  - บัฟเฟอร์สำหรับรับข้อมูล
 * @param   len  - จำนวนไบต์ที่ต้องการอ่าน
 *
 * @return  HAL_OK ถ้าสำเร็จ, HAL_INVALID ถ้าพารามิเตอร์ไม่ถูกต้อง
 */
hal_status_t hal_flash_read(uint32_t addr, void *buf, uint32_t len)
{
    if (!buf || !len) return HAL_INVALID;
    FLASH_ROM_READ(addr, buf, len);
    return HAL_OK;
}

/*********************************************************************
 * @fn      hal_flash_get_unique_id
 *
 * @brief   อ่าน Unique ID (8 ไบต์) ของชิพ CH572
 *
 * @param   buf - บัฟเฟอร์ขนาด 8 ไบต์สำหรับรับ Unique ID
 *
 * @return  HAL_OK ถ้าสำเร็จ, HAL_INVALID ถ้าบัฟเฟอร์เป็น NULL
 */
hal_status_t hal_flash_get_unique_id(uint8_t buf[8])
{
    if (!buf) return HAL_INVALID;
    GET_UNIQUE_ID(buf);
    return HAL_OK;
}

/*********************************************************************
 * @fn      hal_flash_read_config
 *
 * @brief   อ่านค่า configuration word จาก Flash
 *
 * @param   config_val - ตัวชี้ไปยังตัวแปรรับค่า 16 บิต
 *
 * @return  HAL_OK ถ้าสำเร็จ, HAL_INVALID ถ้าตัวชี้เป็น NULL
 */
hal_status_t hal_flash_read_config(uint16_t *config_val)
{
    if (!config_val) return HAL_INVALID;
    uint8_t buf[2];
    hal_flash_read(ROM_CFG_VERISON, buf, 2);
    *config_val = (uint16_t)buf[0] | ((uint16_t)buf[1] << 8);
    return HAL_OK;
}
