/*********************************************************************
 * @file    hal_usbdev.c
 * @brief   SimpleHAL — ตัวขับ USB Device
 *
 * ทำให้ CH572/CH570 เป็น HID Keyboard ผ่าน USB
 * มี descriptors: device, configuration, HID report, string
 * จัดการ standard request (GET_DESCRIPTOR, SET_ADDRESS, SET_CONFIG)
 * และแจ้งเหตุการณ์ผ่าน callback
 */

#include "hal_usbdev.h"

#if HAL_ENABLE_USBDEV

#include <string.h>
#include "CH57x_common.h"

#define USB_DEV_DESCR_SIZE   18
#define USB_CFG_DESCR_SIZE    9
#define USB_INTF_DESCR_SIZE   9
#define USB_HID_DESCR_SIZE    9
#define USB_ENDP_DESCR_SIZE   7

#define HID_REPORT_DESCR_SIZE 63

/* โครงสร้างข้อมูลภายในของ USB Device */
struct hal_usbdev_obj {
    uint8_t used;                    /* flag บอกว่ากำลังใช้งานอยู่ */
    uint8_t configured;              /* flag สถานะ configuration */
    hal_usbdev_callback_t cb;        /* callback เหตุการณ์ */
    void   *user_arg;                /* อาร์กิวเมนต์ที่ส่งให้ callback */
};

static struct hal_usbdev_obj dev_inst;

/* Endpoint buffers (ต้อง align 4 สำหรับ DMA) */
static uint8_t ep0_ram[192] __attribute__((aligned(4)));
static uint8_t ep1_ram[128] __attribute__((aligned(4)));
static uint8_t ep2_ram[128] __attribute__((aligned(4)));
static uint8_t ep3_ram[128] __attribute__((aligned(4)));

static uint8_t config_desc_flag;

/* USB Device Descriptor */
static const uint8_t dev_desc[USB_DEV_DESCR_SIZE] = {
    18,       1, 0x10, 0x01, 0x00, 0x00, 0x00, 8,
    0x34, 0x12, 0x01, 0x00, 0x00, 0x00, 0x01, 0x02,
    0x03, 0x01
};

/* HID Report Descriptor — Keyboard boot protocol */
static const uint8_t hid_report_desc[HID_REPORT_DESCR_SIZE] = {
    0x05, 0x01, 0x09, 0x06, 0xA1, 0x01, 0x05, 0x07,
    0x19, 0xE0, 0x29, 0xE7, 0x15, 0x00, 0x25, 0x01,
    0x75, 0x01, 0x95, 0x08, 0x81, 0x02, 0x95, 0x01,
    0x75, 0x08, 0x81, 0x01, 0x95, 0x05, 0x75, 0x01,
    0x05, 0x08, 0x19, 0x01, 0x29, 0x05, 0x91, 0x02,
    0x95, 0x01, 0x75, 0x03, 0x91, 0x01, 0x95, 0x06,
    0x75, 0x08, 0x15, 0x00, 0x25, 0xFF, 0x05, 0x07,
    0x19, 0x00, 0x29, 0xFF, 0x81, 0x00, 0xC0
};

/* Configuration Descriptor (รวม Interface + HID + Endpoint) */
static const uint8_t cfg_desc[] = {
    USB_CFG_DESCR_SIZE + USB_INTF_DESCR_SIZE + USB_HID_DESCR_SIZE + USB_ENDP_DESCR_SIZE,
    2, 0x29, 0x00, 1, 1, 0, 0xA0, 50,

    USB_INTF_DESCR_SIZE, 4, 0, 0, 1, 3, 1, 1, 0,

    USB_HID_DESCR_SIZE, 0x21, 0x10, 0x01, 0x00, 1, HID_REPORT_DESCR_SIZE & 0xFF,
    (HID_REPORT_DESCR_SIZE >> 8) & 0xFF,

    USB_ENDP_DESCR_SIZE, 5, 0x81, 3, 8, 0x0A
};

/* String Descriptors */
static const uint8_t str_lang[]  = { 4, 3, 0x09, 0x04 };
static const uint8_t str_prod[]  = { 22, 3, 'C',0, 'H',0, '5',0, '7',0, '0',0, ' ',0, 'H',0, 'I',0, 'D',0, 0,0 };

/*********************************************************************
 * @fn      std_req_handler
 *
 * @brief   จัดการ standard USB request (GET_DESCRIPTOR, SET_ADDRESS, SET_CONFIG)
 *
 * @return  ไม่มี
 */
static void std_req_handler(void)
{
    uint8_t len = 0;
    const uint8_t *ptr = 0;

    switch (pSetupReqPak->bRequest) {
    case USB_GET_DESCRIPTOR: {
        uint8_t type = pSetupReqPak->wValue >> 8;
        uint8_t idx  = pSetupReqPak->wValue & 0xFF;
        (void)idx;
        switch (type) {
        case USB_DESCR_TYP_DEVICE:
            ptr = dev_desc;
            len = USB_DEV_DESCR_SIZE;
            break;
        case USB_DESCR_TYP_CONFIG:
            ptr = cfg_desc;
            len = cfg_desc[2] | (cfg_desc[3] << 8);
            config_desc_flag = 1;
            break;
        case USB_DESCR_TYP_HID:
            ptr = cfg_desc + USB_CFG_DESCR_SIZE + USB_INTF_DESCR_SIZE;
            len = USB_HID_DESCR_SIZE;
            break;
        case USB_DESCR_TYP_REPORT:
            ptr = hid_report_desc;
            len = sizeof(hid_report_desc);
            break;
        case USB_DESCR_TYP_STRING:
            if (idx == 0) { ptr = str_lang; len = str_lang[0]; }
            else          { ptr = str_prod; len = str_prod[0]; }
            break;
        }
        break;
    }
    case USB_SET_ADDRESS:
        break;
    case USB_SET_CONFIGURATION:
        config_desc_flag = 0;
        dev_inst.configured = 1;
        if (dev_inst.cb)
            dev_inst.cb(&dev_inst, HAL_USBDEV_EVT_CONFIGURED, dev_inst.user_arg);
        break;
    default:
        if (dev_inst.cb)
            dev_inst.cb(&dev_inst, HAL_USBDEV_EVT_SETUP_REQ, dev_inst.user_arg);
        return;
    }

    if (ptr && len) {
        uint8_t tx_len = len > HAL_USBDEV_EP_MAX_PACKET ? HAL_USBDEV_EP_MAX_PACKET : len;
        memcpy(pEP0_DataBuf, ptr, tx_len);
        R8_UEP0_CTRL = (R8_UEP0_CTRL & ~MASK_UEP_T_RES) | UEP_T_RES_ACK;
    }
}

/*********************************************************************
 * @fn      USB_DevTransProcess
 *
 * @brief   ฟังก์ชันจัดการ USB transfer หลัก (เรียกจาก hal_usbdev_int_handler)
 *
 * จัดการ:
 *   - Bus reset → แจ้ง HAL_USBDEV_EVT_RESET
 *   - Suspend   → แจ้ง HAL_USBDEV_EVT_SUSPEND
 *   - Setup     → เรียก std_req_handler
 *   - IN/OUT    → แจ้งเหตุการณ์ผ่าน callback
 *
 * @return  ไม่มี
 */
void USB_DevTransProcess(void)
{
    uint8_t setup_req = R8_USB_INT_ST & MASK_UIS_TOKEN;

    if (R8_USB_INT_FG & RB_UIF_BUS_RST) {
        R8_USB_INT_FG = RB_UIF_BUS_RST;
        dev_inst.configured = 0;
        if (dev_inst.cb)
            dev_inst.cb(&dev_inst, HAL_USBDEV_EVT_RESET, dev_inst.user_arg);
        return;
    }

    if (R8_USB_INT_FG & RB_UIF_SUSPEND) {
        R8_USB_INT_FG = RB_UIF_SUSPEND;
        if (dev_inst.cb)
            dev_inst.cb(&dev_inst, HAL_USBDEV_EVT_SUSPEND, dev_inst.user_arg);
        return;
    }

    if (!(R8_USB_INT_FG & RB_UIF_TRANSFER)) return;
    R8_USB_INT_FG = RB_UIF_TRANSFER;

    switch (setup_req) {
    case UIS_TOKEN_SETUP:
        R8_UEP0_CTRL = (R8_UEP0_CTRL & ~MASK_UEP_R_RES) | UEP_R_RES_ACK;
        R8_UEP0_CTRL = (R8_UEP0_CTRL & ~MASK_UEP_T_RES) | UEP_T_RES_NAK;
        std_req_handler();
        break;

    case UIS_TOKEN_IN:
        if (R8_UEP0_CTRL & UEP_T_RES_NAK) {
            R8_UEP0_CTRL = (R8_UEP0_CTRL & ~MASK_UEP_T_RES) | UEP_T_RES_NAK;
        }
        break;

    case UIS_TOKEN_OUT: {
        uint8_t ep = (R8_USB_INT_ST & MASK_UIS_ENDP);
        if (ep >= 1 && ep <= HAL_USBDEV_NUM_ENDPOINTS) {
            static const hal_usbdev_evt_t out_evts[] = {
                HAL_USBDEV_EVT_EP1_OUT,
                HAL_USBDEV_EVT_EP2_OUT,
                HAL_USBDEV_EVT_EP3_OUT,
                HAL_USBDEV_EVT_EP4_OUT
            };
            if (dev_inst.cb)
                dev_inst.cb(&dev_inst, out_evts[ep - 1], dev_inst.user_arg);
        }
        break;
    }
    }
}

/*********************************************************************
 * @fn      hal_usbdev_init
 *
 * @brief   เริ่มต้น USB Device: กำหนด endpoint buffer และเปิดอุปกรณ์
 *
 * @return  handle ของ USB Device
 */
hal_usbdev_handle_t hal_usbdev_init(void)
{
    struct hal_usbdev_obj *h = &dev_inst;

    if (h->used) return h;

    memset(h, 0, sizeof(*h));

    pEP0_RAM_Addr = ep0_ram;    /* กำหนด buffer สำหรับ EP0 */
    pEP1_RAM_Addr = ep1_ram;    /* กำหนด buffer สำหรับ EP1 */
    pEP2_RAM_Addr = ep2_ram;    /* กำหนด buffer สำหรับ EP2 */
    pEP3_RAM_Addr = ep3_ram;    /* กำหนด buffer สำหรับ EP3 */

    USB_DeviceInit();            /* เริ่มต้น device USB */

    h->used = 1;
    return h;
}

/*********************************************************************
 * @fn      hal_usbdev_deinit
 *
 * @brief   ปิด USB Device: ล้างค่าและปิดสัญญาณ USB
 *
 * @param   h - handle ของ USB Device
 *
 * @return  ไม่มี
 */
void hal_usbdev_deinit(hal_usbdev_handle_t h)
{
    if (!h || !h->used) return;

    R8_USB_CTRL = 0;
    R16_PIN_ALTERNATE &= ~RB_PIN_USB_EN;

    memset(h, 0, sizeof(*h));
}

/*********************************************************************
 * @fn      hal_usbdev_send
 *
 * @brief   ส่งข้อมูลไปยัง Host ผ่าน endpoint IN
 *
 * @param   h    - handle ของ USB Device
 * @param   ep   - หมายเลข endpoint (1-4)
 * @param   data - ตัวชี้ข้อมูลที่จะส่ง
 * @param   len  - จำนวนไบต์ที่ต้องการส่ง
 *
 * @return  ไม่มี
 */
void hal_usbdev_send(hal_usbdev_handle_t h, uint8_t ep, const uint8_t *data, uint8_t len)
{
    if (!h || !h->used || ep < 1 || ep > HAL_USBDEV_NUM_ENDPOINTS) return;
    if (!data || len > HAL_USBDEV_EP_MAX_PACKET) return;

    uint8_t *dest;
    switch (ep) {
    case 1: dest = pEP1_IN_DataBuf; break;
    case 2: dest = pEP2_IN_DataBuf; break;
    case 3: dest = pEP3_IN_DataBuf; break;
    case 4: dest = pEP4_IN_DataBuf; break;
    default: return;
    }

    memcpy(dest, data, len);

    switch (ep) {
    case 1: DevEP1_IN_Deal(len); break;
    case 2: DevEP2_IN_Deal(len); break;
    case 3: DevEP3_IN_Deal(len); break;
    case 4: DevEP4_IN_Deal(len); break;
    }
}

/*********************************************************************
 * @fn      hal_usbdev_in_ready
 *
 * @brief   ตรวจสอบว่า endpoint IN พร้อมส่งข้อมูลหรือยัง
 *
 * @param   h  - handle ของ USB Device
 * @param   ep - หมายเลข endpoint (1-4)
 *
 * @return  1 = พร้อมส่ง (TX สล็อตว่าง), 0 = กำลังส่ง
 */
uint8_t hal_usbdev_in_ready(hal_usbdev_handle_t h, uint8_t ep)
{
    if (!h || !h->used) return 0;

    switch (ep) {
    case 1: return EP1_GetINSta();
    case 2: return EP2_GetINSta();
    case 3: return EP3_GetINSta();
    case 4: return EP4_GetINSta();
    default: return 0;
    }
}

/*********************************************************************
 * @fn      hal_usbdev_get_recv_data
 *
 * @brief   อ่านข้อมูลที่รับจาก Host ผ่าน endpoint OUT
 *
 * @param   h   - handle ของ USB Device
 * @param   ep  - หมายเลข endpoint (1-4)
 * @param   len - ตัวชี้รับขนาดข้อมูลที่ได้รับ
 *
 * @return  ตัวชี้ไปยัง buffer ที่มีข้อมูล หรือ NULL ถ้าไม่มีข้อมูล
 */
uint8_t *hal_usbdev_get_recv_data(hal_usbdev_handle_t h, uint8_t ep, uint8_t *len)
{
    if (!h || !h->used || !len) return 0;

    uint8_t *src;
    switch (ep) {
    case 1: src = pEP1_OUT_DataBuf; break;
    case 2: src = pEP2_OUT_DataBuf; break;
    case 3: src = pEP3_OUT_DataBuf; break;
    case 4: src = pEP4_OUT_DataBuf; break;
    default: return 0;
    }

    *len = HAL_USBDEV_EP_MAX_PACKET;
    return src;
}

/*********************************************************************
 * @fn      hal_usbdev_set_configurated
 *
 * @brief   ตั้งค่า flag ว่า device ถูกรับการตั้งค่าแล้ว
 *
 * @param   h - handle ของ USB Device
 *
 * @return  ไม่มี
 */
void hal_usbdev_set_configurated(hal_usbdev_handle_t h)
{
    if (!h || !h->used) return;
    h->configured = 1;
}

/*********************************************************************
 * @fn      hal_usbdev_attach_cb
 *
 * @brief   ผูก callback สำหรับเหตุการณ์ USB Device
 *
 * @param   h        - handle ของ USB Device
 * @param   cb       - ฟังก์ชัน callback
 * @param   user_arg - อาร์กิวเมนต์ที่ส่งให้ callback
 *
 * @return  ไม่มี
 */
void hal_usbdev_attach_cb(hal_usbdev_handle_t h, hal_usbdev_callback_t cb, void *user_arg)
{
    if (!h || !h->used) return;
    h->cb       = cb;
    h->user_arg = user_arg;
}

/*********************************************************************
 * @fn      hal_usbdev_int_handler
 *
 * @brief   ตัวจัดการขัดจังหวะ USB Device หลัก
 *
 * เรียก USB_DevTransProcess() เพื่อจัดการ transfer
 *
 * @return  ไม่มี
 */
void hal_usbdev_int_handler(void)
{
    USB_DevTransProcess();
}

#endif
