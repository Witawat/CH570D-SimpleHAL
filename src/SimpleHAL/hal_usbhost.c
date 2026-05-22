/*********************************************************************
 * @file    hal_usbhost.c
 * @brief   SimpleHAL — ตัวขับ USB Host
 *
 * ใช้งาน WCH USB Host library (CH57x_usbhostBase.c, CH57x_usbhostClass.c)
 * รองรับคีย์บอร์ด HID, มี callback สำหรับ connect/disconnect/error
 */

#include "hal_usbhost.h"

#if HAL_ENABLE_USBHOST

#include <string.h>
#include "CH57x_common.h"

/* โครงสร้างข้อมูลภายในของ USB Host */
struct hal_usbhost_obj {
    uint8_t  used;                    /* flag บอกว่ากำลังใช้งานอยู่ */
    uint8_t  connected;               /* flag สถานะเชื่อมต่อ */
    hal_usbhost_dev_type_t dev_type;  /* ประเภทอุปกรณ์ */
    uint16_t vid;                     /* Vendor ID */
    uint16_t pid;                     /* Product ID */
    hal_usbhost_callback_t cb;        /* callback เหตุการณ์ */
    void    *user_arg;                /* อาร์กิวเมนต์ที่ส่งให้ callback */
};

static struct hal_usbhost_obj host_inst;
static uint8_t host_rx_buf[HAL_USBHOST_DMA_BUF_SIZE] __attribute__((aligned(4)));
static uint8_t host_tx_buf[HAL_USBHOST_DMA_BUF_SIZE] __attribute__((aligned(4)));

/*********************************************************************
 * @fn      map_dev_type
 *
 * @brief   แปลงประเภทอุปกรณ์จาก WCH SDK เป็น hal_usbhost_dev_type_t
 *
 * @param   raw_type - ประเภทดิบจาก ThisUsbDev.DeviceType
 *
 * @return  ประเภทที่ mapped แล้ว
 */
static hal_usbhost_dev_type_t map_dev_type(uint8_t raw_type)
{
    switch (raw_type) {
    case DEV_TYPE_KEYBOARD: return HAL_USBHOST_DEV_KEYBOARD;
    case DEV_TYPE_MOUSE:    return HAL_USBHOST_DEV_MOUSE;
    default:
        if (raw_type == USB_DEV_CLASS_HUB)     return HAL_USBHOST_DEV_HUB;
        if (raw_type == USB_DEV_CLASS_STORAGE)  return HAL_USBHOST_DEV_STORAGE;
        if (raw_type == USB_DEV_CLASS_PRINTER)  return HAL_USBHOST_DEV_PRINTER;
        return HAL_USBHOST_DEV_UNKNOWN;
    }
}

/*********************************************************************
 * @fn      hal_usbhost_init
 *
 * @brief   เริ่มต้น USB Host: เปิดสัญญาณ USB และตั้งค่า DMA buffer
 *
 * @return  handle ของ USB Host
 */
hal_usbhost_handle_t hal_usbhost_init(void)
{
    struct hal_usbhost_obj *h = &host_inst;

    if (h->used) return h;

    memset(h, 0, sizeof(*h));

    R16_PIN_ALTERNATE |= RB_PIN_USB_EN;     /* เปิดสัญญาณ USB */

    pHOST_RX_RAM_Addr = host_rx_buf;        /* กำหนด DMA buffer รับ */
    pHOST_TX_RAM_Addr = host_tx_buf;        /* กำหนด DMA buffer ส่ง */

    USB_HostInit();                         /* เริ่มต้นโฮสต์ USB */

    h->used = 1;
    return h;
}

/*********************************************************************
 * @fn      hal_usbhost_deinit
 *
 * @brief   ปิด USB Host: ล้างค่าและปิดสัญญาณ USB
 *
 * @param   h - handle ของ USB Host
 *
 * @return  ไม่มี
 */
void hal_usbhost_deinit(hal_usbhost_handle_t h)
{
    if (!h || !h->used) return;

    R8_USB_CTRL = 0;
    R16_PIN_ALTERNATE &= ~RB_PIN_USB_EN;

    memset(h, 0, sizeof(*h));
}

/*********************************************************************
 * @fn      hal_usbhost_poll
 *
 * @brief   ตรวจสอบสถานะ USB (เรียกต่อเนื่องใน loop หลัก)
 *
 * @param   h - handle ของ USB Host
 *
 * @return  เหตุการณ์ล่าสุด (CONNECT, DISCONNECT, READY, ERROR, NONE)
 */
hal_usbhost_evt_t hal_usbhost_poll(hal_usbhost_handle_t h)
{
    uint8_t s;

    if (!h || !h->used) return HAL_USBHOST_EVT_NONE;

    s = AnalyzeRootHub();

    /* อุปกรณ์เชื่อมต่อใหม่ */
    if (s == ERR_USB_CONNECT) {
        ResetRootHubPort();
        EnableRootHubPort();

        if (InitRootDevice() == ERR_SUCCESS &&
            ThisUsbDev.DeviceStatus == ROOT_DEV_SUCCESS) {

            h->connected = 1;
            h->dev_type  = map_dev_type(ThisUsbDev.DeviceType);
            h->vid       = ThisUsbDev.DeviceVID;
            h->pid       = ThisUsbDev.DevicePID;

            if (h->dev_type == HAL_USBHOST_DEV_HUB) {
                EnumAllHubPort();
            }

            if (h->cb) h->cb(h, HAL_USBHOST_EVT_READY, h->user_arg);
            return HAL_USBHOST_EVT_READY;
        }

        h->connected = 0;
        if (h->cb) h->cb(h, HAL_USBHOST_EVT_ERROR, h->user_arg);
        return HAL_USBHOST_EVT_ERROR;
    }

    /* อุปกรณ์ถูกถอด */
    if (s == ERR_USB_DISCON) {
        h->connected = 0;
        h->dev_type  = HAL_USBHOST_DEV_NONE;
        h->vid       = 0;
        h->pid       = 0;

        if (h->cb) h->cb(h, HAL_USBHOST_EVT_DISCONNECT, h->user_arg);
        return HAL_USBHOST_EVT_DISCONNECT;
    }

    if (h->connected && h->dev_type == HAL_USBHOST_DEV_HUB) {
        EnumAllHubPort();
    }

    return HAL_USBHOST_EVT_NONE;
}

/*********************************************************************
 * @fn      hal_usbhost_get_device_type
 *
 * @brief   อ่านประเภทอุปกรณ์ที่เชื่อมต่อ
 *
 * @param   h - handle ของ USB Host
 *
 * @return  ประเภทอุปกรณ์
 */
hal_usbhost_dev_type_t hal_usbhost_get_device_type(hal_usbhost_handle_t h)
{
    if (!h || !h->used) return HAL_USBHOST_DEV_NONE;
    return h->dev_type;
}

/*********************************************************************
 * @fn      hal_usbhost_get_vid
 *
 * @brief   อ่าน Vendor ID ของอุปกรณ์
 *
 * @param   h - handle ของ USB Host
 *
 * @return  VID
 */
uint16_t hal_usbhost_get_vid(hal_usbhost_handle_t h)
{
    if (!h || !h->used) return 0;
    return h->vid;
}

/*********************************************************************
 * @fn      hal_usbhost_get_pid
 *
 * @brief   อ่าน Product ID ของอุปกรณ์
 *
 * @param   h - handle ของ USB Host
 *
 * @return  PID
 */
uint16_t hal_usbhost_get_pid(hal_usbhost_handle_t h)
{
    if (!h || !h->used) return 0;
    return h->pid;
}

/*********************************************************************
 * @fn      hal_usbhost_is_ready
 *
 * @brief   ตรวจสอบว่าอุปกรณ์พร้อมทำงาน
 *
 * @param   h - handle ของ USB Host
 *
 * @return  1 = พร้อม, 0 = ยังไม่พร้อม
 */
uint8_t hal_usbhost_is_ready(hal_usbhost_handle_t h)
{
    if (!h || !h->used) return 0;
    return h->connected;
}

/*********************************************************************
 * @fn      hal_usbhost_kbd_read
 *
 * @brief   อ่านรายงาน HID จากคีย์บอร์ด
 *
 * @param   h      - handle ของ USB Host
 * @param   report - ตัวชี้ไปยังโครงสร้างรับรายงาน
 *
 * @return  HAL_OK ถ้าสำเร็จ, HAL_BUSY ถ้า NAK, HAL_ERROR ถ้าผิดพลาด
 */
hal_status_t hal_usbhost_kbd_read(hal_usbhost_handle_t h, hal_usbhost_kbd_report_t *report)
{
    uint8_t s;

    if (!h || !h->used) return HAL_INVALID;
    if (!h->connected || h->dev_type != HAL_USBHOST_DEV_KEYBOARD) return HAL_BUSY;

    SelectHubPort(0);

    s = USBHostTransact(
        (USB_PID_IN << 4) | ThisUsbDev.GpVar[0],
        RB_UH_R_AUTO_TOG,
        0);

    if (s == ERR_SUCCESS) {
        uint8_t len = R8_USB_RX_LEN;
        if (len > sizeof(hal_usbhost_kbd_report_t))
            len = sizeof(hal_usbhost_kbd_report_t);
        memcpy(report, pHOST_RX_RAM_Addr, len);
        return HAL_OK;
    }

    if ((s & 0x0F) == USB_PID_NAK) {
        return HAL_BUSY;
    }

    return HAL_ERROR;
}

/*********************************************************************
 * @fn      hal_usbhost_attach_cb
 *
 * @brief   ผูก callback สำหรับเหตุการณ์ USB Host
 *
 * @param   h        - handle ของ USB Host
 * @param   cb       - ฟังก์ชัน callback
 * @param   user_arg - อาร์กิวเมนต์ที่ส่งให้ callback
 *
 * @return  ไม่มี
 */
void hal_usbhost_attach_cb(hal_usbhost_handle_t h, hal_usbhost_callback_t cb, void *user_arg)
{
    if (!h || !h->used) return;
    h->cb       = cb;
    h->user_arg = user_arg;
}

#endif
