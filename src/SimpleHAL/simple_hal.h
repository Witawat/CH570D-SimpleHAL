/*
 * simple_hal.h -- ไฟล์รวมทุกโมดูลของ SimpleHAL
 *
 * เพียง include ไฟล์นี้ในแอปพลิเคชันก็ใช้ทุกโมดูลได้ทันที
 *
 * การเปิด/ปิดโมดูล (กำหนดใน simple_hal_config.h):
 * - HAL_ENABLE_BLE = 0 (ปิด ต้องใช้ WCH BLE library แยก)
 * - HAL_ENABLE_RF  = 0 (ปิด ต้องใช้ WCH RF library แยก)
 */

#ifndef __SIMPLE_HAL_H__
#define __SIMPLE_HAL_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "simple_hal_config.h"
#include "CH57x_common.h"

#include "core/hal_types.h"
#include "core/hal_utils.h"
#include "core/hal_ringbuf.h"
#include "core/hal_softimer.h"
#include "hal_gpio.h"
#include "hal_uart.h"
#include "hal_spi.h"
#include "hal_i2c.h"
#include "hal_timer.h"
#include "hal_pwm.h"
#include "hal_adc.h"
#include "hal_flash.h"
#include "hal_rtc.h"
#include "hal_pwr.h"
#include "hal_clk.h"
#include "hal_sys.h"
#include "hal_keyscan.h"
#include "hal_cmp.h"
#if HAL_ENABLE_RF
#include "hal_rf.h"
#endif
#if HAL_ENABLE_BLE
#include "hal_ble.h"
#endif
#if HAL_ENABLE_USBHOST
#include "hal_usbhost.h"
#endif
#if HAL_ENABLE_USBDEV
#include "hal_usbdev.h"
#endif

#ifdef __cplusplus
}
#endif

#endif
