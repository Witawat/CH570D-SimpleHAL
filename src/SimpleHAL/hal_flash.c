#include "hal_flash.h"
#include "CH57x_common.h"

hal_status_t hal_flash_read(uint32_t addr, void *buf, uint32_t len)
{
    if (!buf || !len) return HAL_INVALID;
    FLASH_ROM_READ(addr, buf, len);
    return HAL_OK;
}

hal_status_t hal_flash_get_unique_id(uint8_t buf[8])
{
    if (!buf) return HAL_INVALID;
    GET_UNIQUE_ID(buf);
    return HAL_OK;
}

hal_status_t hal_flash_read_config(uint16_t *config_val)
{
    if (!config_val) return HAL_INVALID;
    uint8_t buf[2];
    hal_flash_read(ROM_CFG_VERISON, buf, 2);
    *config_val = (uint16_t)buf[0] | ((uint16_t)buf[1] << 8);
    return HAL_OK;
}
