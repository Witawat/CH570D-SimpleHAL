#ifndef __HAL_I2C_H__
#define __HAL_I2C_H__

#include <stdint.h>
#include "core/hal_types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define HAL_I2C_MAX_INSTANCES 1

typedef enum {
    HAL_I2C_MASTER = 0,
    HAL_I2C_SLAVE,
} hal_i2c_role_t;

typedef enum {
    HAL_I2C_SPEED_STANDARD = 100000,
    HAL_I2C_SPEED_FAST     = 400000,
} hal_i2c_speed_t;

typedef struct hal_i2c_obj *hal_i2c_handle_t;

hal_i2c_handle_t hal_i2c_init(uint32_t clock_hz, hal_i2c_role_t role, uint16_t own_addr);
hal_status_t     hal_i2c_write(hal_i2c_handle_t h, uint8_t dev_addr, uint8_t reg, const uint8_t *data, uint16_t len);
hal_status_t     hal_i2c_read(hal_i2c_handle_t h, uint8_t dev_addr, uint8_t reg, uint8_t *data, uint16_t len);
hal_status_t     hal_i2c_write_raw(hal_i2c_handle_t h, uint8_t dev_addr, const uint8_t *data, uint16_t len);
hal_status_t     hal_i2c_read_raw(hal_i2c_handle_t h, uint8_t dev_addr, uint8_t *data, uint16_t len);
hal_status_t     hal_i2c_mem_write(hal_i2c_handle_t h, uint8_t dev_addr, uint16_t mem_addr, uint8_t addr_len, const uint8_t *data, uint16_t len);
hal_status_t     hal_i2c_mem_read(hal_i2c_handle_t h, uint8_t dev_addr, uint16_t mem_addr, uint8_t addr_len, uint8_t *data, uint16_t len);
void             hal_i2c_set_speed(hal_i2c_handle_t h, uint32_t clock_hz);

#ifdef __cplusplus
}
#endif

#endif
