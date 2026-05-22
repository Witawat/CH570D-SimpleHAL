/*********************************************************************
 * @file    hal_usbhost.h
 * @brief   SimpleHAL — โมดูล USB Host
 *
 * รองรับการต่อพ่วงอุปกรณ์ USB (คีย์บอร์ด, เมาส์, Hub, Storage, Printer)
 * ทำงานผ่าน WCH USB Host library (CH57x_usbhostBase/Class.c)
 */

#ifndef __HAL_USBHOST_H__
#define __HAL_USBHOST_H__

#include "simple_hal_config.h"

#if HAL_ENABLE_USBHOST

#include "core/hal_types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define HAL_USBHOST_MAX_INSTANCES   1
#define HAL_USBHOST_DMA_BUF_SIZE   64

/**
 * @brief   ประเภทอุปกรณ์ USB ที่ตรวจพบ
 */
typedef enum {
    HAL_USBHOST_DEV_NONE      = 0,  /* ไม่พบอุปกรณ์ */
    HAL_USBHOST_DEV_KEYBOARD  = 1,  /* คีย์บอร์ด */
    HAL_USBHOST_DEV_MOUSE     = 2,  /* เมาส์ */
    HAL_USBHOST_DEV_HUB       = 3,  /* USB Hub */
    HAL_USBHOST_DEV_STORAGE   = 4,  /* USB Storage */
    HAL_USBHOST_DEV_PRINTER   = 5,  /* เครื่องพิมพ์ */
    HAL_USBHOST_DEV_UNKNOWN   = 6,  /* อุปกรณ์ไม่รู้จัก */
} hal_usbhost_dev_type_t;

/**
 * @brief   เหตุการณ์จาก USB Host
 */
typedef enum {
    HAL_USBHOST_EVT_NONE        = 0,  /* ไม่มีเหตุการณ์ */
    HAL_USBHOST_EVT_CONNECT     = 1,  /* มีอุปกรณ์เชื่อมต่อ */
    HAL_USBHOST_EVT_DISCONNECT  = 2,  /* อุปกรณ์ถูกถอด */
    HAL_USBHOST_EVT_READY       = 3,  /* อุปกรณ์พร้อมทำงาน */
    HAL_USBHOST_EVT_ERROR       = 4,  /* เกิดข้อผิดพลาด */
} hal_usbhost_evt_t;

/**
 * @brief   โครงสร้างรายงานคีย์บอร์ด HID (8 bytes)
 */
typedef struct {
    uint8_t modifiers;   /* ปุ่มปรับสถานะ (Ctrl, Alt, Shift, GUI) */
    uint8_t reserved;
    uint8_t keys[6];     /* คีย์ที่ถูกกด สูงสุด 6 พร้อมกัน */
} hal_usbhost_kbd_report_t;

typedef struct hal_usbhost_obj *hal_usbhost_handle_t;

typedef void (*hal_usbhost_callback_t)(hal_usbhost_handle_t h, hal_usbhost_evt_t evt, void *user_arg);

/**
 * @brief   เริ่มต้น USB Host: เปิดสัญญาณ USB, จัดสรร DMA buffer
 *
 * @return  handle ของ USB Host
 */
hal_usbhost_handle_t hal_usbhost_init(void);

/**
 * @brief   ปิด USB Host
 *
 * @param   h - handle ของ USB Host
 */
void hal_usbhost_deinit(hal_usbhost_handle_t h);

/**
 * @brief   ตรวจสอบและจัดการสถานะ USB (เรียกเป็นระยะ)
 *
 * @param   h - handle ของ USB Host
 *
 * @return  เหตุการณ์ล่าสุด
 */
hal_usbhost_evt_t hal_usbhost_poll(hal_usbhost_handle_t h);

/**
 * @brief   อ่านประเภทอุปกรณ์ที่เชื่อมต่อ
 *
 * @param   h - handle ของ USB Host
 *
 * @return  ประเภทอุปกรณ์
 */
hal_usbhost_dev_type_t hal_usbhost_get_device_type(hal_usbhost_handle_t h);

/**
 * @brief   อ่าน Vendor ID ของอุปกรณ์
 *
 * @param   h - handle ของ USB Host
 *
 * @return  VID
 */
uint16_t hal_usbhost_get_vid(hal_usbhost_handle_t h);

/**
 * @brief   อ่าน Product ID ของอุปกรณ์
 *
 * @param   h - handle ของ USB Host
 *
 * @return  PID
 */
uint16_t hal_usbhost_get_pid(hal_usbhost_handle_t h);

/**
 * @brief   ตรวจสอบว่าอุปกรณ์พร้อมทำงานหรือไม่
 *
 * @param   h - handle ของ USB Host
 *
 * @return  1 = พร้อม, 0 = ยังไม่พร้อม
 */
uint8_t hal_usbhost_is_ready(hal_usbhost_handle_t h);

/**
 * @brief   อ่านรายงานคีย์บอร์ด HID (ใช้กับคีย์บอร์ดเท่านั้น)
 *
 * @param   h      - handle ของ USB Host
 * @param   report - ตัวชี้ไปยังโครงสร้างรับรายงาน
 *
 * @return  HAL_OK ถ้าสำเร็จ, HAL_BUSY ถ้ายังไม่พร้อม, HAL_ERROR ถ้าเกิดข้อผิดพลาด
 */
hal_status_t hal_usbhost_kbd_read(hal_usbhost_handle_t h, hal_usbhost_kbd_report_t *report);

/**
 * @brief   ผูก callback สำหรับเหตุการณ์ USB Host
 *
 * @param   h        - handle ของ USB Host
 * @param   cb       - ฟังก์ชัน callback
 * @param   user_arg - อาร์กิวเมนต์ที่ส่งให้ callback
 */
void hal_usbhost_attach_cb(hal_usbhost_handle_t h, hal_usbhost_callback_t cb, void *user_arg);

#ifdef __cplusplus
}
#endif

#endif

#endif
