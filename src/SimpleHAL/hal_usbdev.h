/*********************************************************************
 * @file    hal_usbdev.h
 * @brief   SimpleHAL — โมดูล USB Device
 *
 * ให้ CH572/CH570 ทำหน้าที่เป็นอุปกรณ์ USB HID Keyboard
 * รองรับ standard request, HID report, และ multi-endpoint OUT
 */

#ifndef __HAL_USBDEV_H__
#define __HAL_USBDEV_H__

#include "simple_hal_config.h"

#if HAL_ENABLE_USBDEV

#include "core/hal_types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define HAL_USBDEV_MAX_INSTANCES       1
#define HAL_USBDEV_EP_MAX_PACKET      64
#define HAL_USBDEV_NUM_ENDPOINTS       4

/**
 * @brief   เหตุการณ์จาก USB Device
 */
typedef enum {
    HAL_USBDEV_EVT_RESET       = 0,  /* USB bus reset */
    HAL_USBDEV_EVT_SUSPEND     = 1,  /* USB suspend */
    HAL_USBDEV_EVT_CONFIGURED  = 2,  /* Host ตั้งค่า configuration แล้ว */
    HAL_USBDEV_EVT_SETUP_REQ   = 3,  /* มี standard request ที่ไม่รู้จัก */
    HAL_USBDEV_EVT_EP1_OUT     = 4,  /* รับข้อมูลจาก EP1 OUT */
    HAL_USBDEV_EVT_EP2_OUT     = 5,  /* รับข้อมูลจาก EP2 OUT */
    HAL_USBDEV_EVT_EP3_OUT     = 6,  /* รับข้อมูลจาก EP3 OUT */
    HAL_USBDEV_EVT_EP4_OUT     = 7,  /* รับข้อมูลจาก EP4 OUT */
} hal_usbdev_evt_t;

typedef struct hal_usbdev_obj *hal_usbdev_handle_t;

/**
 * @brief   โครงสร้างรายงานคีย์บอร์ด HID (8 bytes)
 */
typedef struct {
    uint8_t modifiers;   /* ปุ่มปรับสถานะ (Ctrl, Alt, Shift, GUI) */
    uint8_t reserved;
    uint8_t keys[6];     /* คีย์ที่ถูกกด สูงสุด 6 พร้อมกัน */
} hal_usbdev_kbd_report_t;

typedef void (*hal_usbdev_callback_t)(hal_usbdev_handle_t h, hal_usbdev_evt_t evt, void *user_arg);

/**
 * @brief   เริ่มต้น USB Device: เปิดสัญญาณ USB, กำหนด endpoint buffer
 *
 * @return  handle ของ USB Device
 */
hal_usbdev_handle_t hal_usbdev_init(void);

/**
 * @brief   ปิด USB Device
 *
 * @param   h - handle ของ USB Device
 */
void hal_usbdev_deinit(hal_usbdev_handle_t h);

/**
 * @brief   ส่งข้อมูลไปยัง Host ผ่าน endpoint ที่กำหนด
 *
 * @param   h    - handle ของ USB Device
 * @param   ep   - หมายเลข endpoint (1-4)
 * @param   data - ตัวชี้ข้อมูลที่จะส่ง
 * @param   len  - จำนวนไบต์ที่จะส่ง
 */
void hal_usbdev_send(hal_usbdev_handle_t h, uint8_t ep, const uint8_t *data, uint8_t len);

/**
 * @brief   ตรวจสอบว่า endpoint IN พร้อมส่งข้อมูลหรือยัง
 *
 * @param   h  - handle ของ USB Device
 * @param   ep - หมายเลข endpoint (1-4)
 *
 * @return  1 = พร้อม, 0 = ยังไม่พร้อม
 */
uint8_t hal_usbdev_in_ready(hal_usbdev_handle_t h, uint8_t ep);

/**
 * @brief   อ่านข้อมูลที่รับจาก Host ผ่าน endpoint OUT
 *
 * @param   h   - handle ของ USB Device
 * @param   ep  - หมายเลข endpoint (1-4)
 * @param   len - ตัวชี้รับขนาดข้อมูล
 *
 * @return  ตัวชี้ไปยัง buffer ที่มีข้อมูล หรือ NULL ถ้าไม่มีข้อมูล
 */
uint8_t *hal_usbdev_get_recv_data(hal_usbdev_handle_t h, uint8_t ep, uint8_t *len);

/**
 * @brief   ตั้งค่าว่า device ถูก Host ตั้งค่า configuration แล้ว
 *
 * @param   h - handle ของ USB Device
 */
void hal_usbdev_set_configurated(hal_usbdev_handle_t h);

/**
 * @brief   ผูก callback สำหรับเหตุการณ์ USB Device
 *
 * @param   h        - handle ของ USB Device
 * @param   cb       - ฟังก์ชัน callback
 * @param   user_arg - อาร์กิวเมนต์ที่ส่งให้ callback
 */
void hal_usbdev_attach_cb(hal_usbdev_handle_t h, hal_usbdev_callback_t cb, void *user_arg);

/**
 * @brief   ตัวจัดการขัดจังหวะ USB Device หลัก เรียกจาก ISR
 *
 * จัดการ USB bus reset, suspend, setup request, และ data transfer
 */
void hal_usbdev_int_handler(void);

#ifdef __cplusplus
}
#endif

#endif

#endif
