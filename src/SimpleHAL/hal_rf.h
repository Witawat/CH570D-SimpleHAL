#ifndef __HAL_RF_H__
#define __HAL_RF_H__

#include <stdint.h>
#include "core/hal_types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define HAL_RF_MAX_INSTANCES 1

typedef enum {
    HAL_RF_PHY_1M  = 0,
    HAL_RF_PHY_2M  = 1,
    HAL_RF_PHY_2G4 = 2,
} hal_rf_phy_mode_t;

typedef struct hal_rf_obj *hal_rf_handle_t;

hal_rf_handle_t hal_rf_init(uint32_t frequency_khz, hal_rf_phy_mode_t phy, int8_t tx_power_dbm);
hal_status_t    hal_rf_send(hal_rf_handle_t h, const uint8_t *data, uint16_t len);
hal_status_t    hal_rf_recv(hal_rf_handle_t h, uint8_t *buf, uint16_t max_len, uint16_t *out_len, int8_t *rssi, uint32_t timeout_us);
void            hal_rf_set_tx_power(hal_rf_handle_t h, int8_t power_dbm);
void            hal_rf_attach_rx_cb(hal_rf_handle_t h, hal_callback_t cb, void *arg);
void            hal_rf_stop(hal_rf_handle_t h);
void            hal_rf_sleep(hal_rf_handle_t h);
void            hal_rf_wake(hal_rf_handle_t h);
void            hal_rf_set_channel(hal_rf_handle_t h, uint32_t frequency_khz);
void            hal_rf_calibrate(hal_rf_handle_t h);

#ifdef __cplusplus
}
#endif

#endif
