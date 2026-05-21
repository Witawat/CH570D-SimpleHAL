#ifndef __HAL_FLASH_H__
#define __HAL_FLASH_H__

#include <stdint.h>
#include "core/hal_types.h"

#ifdef __cplusplus
extern "C" {
#endif

hal_status_t hal_flash_read(uint32_t addr, void *buf, uint32_t len);
hal_status_t hal_flash_get_unique_id(uint8_t buf[8]);
hal_status_t hal_flash_read_config(uint16_t *config_val);

#ifdef __cplusplus
}
#endif

#endif
