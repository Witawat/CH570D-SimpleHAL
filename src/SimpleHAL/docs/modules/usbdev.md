# SimpleHAL — USB Device Module

**ไฟล์:** `hal_usbdev.h` / `hal_usbdev.c`

โหมด USB Device — ให้ MCU ทำงานเป็นอุปกรณ์ USB HID Keyboard เมื่อต่อกับ PC หรือ smartphone

**ข้อกำหนดทางฮาร์ดแวร์:**
- ต่อ UDP (D+) และ UDM (D-) ของ MCU เข้ากับ USB Micro-B หรือ USB-C
- ใช้ internal pull-up resistor (เปิดให้โดย `hal_usbdev_init()`)
- ไม่ต้องใช้ VBUS 5V ภายนอก — PC จ่ายไฟผ่าน USB

**การเปิดใช้งาน:** ตั้งค่า `HAL_ENABLE_USBDEV = 1` ใน `simple_hal_config.h`
> ⚠️ ห้ามเปิด `HAL_ENABLE_USBDEV` และ `HAL_ENABLE_USBHOST` พร้อมกัน

---

## รูปแบบข้อมูล

### hal_usbdev_evt_t

```c
typedef enum {
    HAL_USBDEV_EVT_RESET       = 0,   // USB bus reset
    HAL_USBDEV_EVT_SUSPEND     = 1,   // USB suspend
    HAL_USBDEV_EVT_CONFIGURED  = 2,   // Host ตั้งค่า configuration แล้ว
    HAL_USBDEV_EVT_SETUP_REQ   = 3,   // Setup request นอกเหนือจาก standard
    HAL_USBDEV_EVT_EP1_OUT     = 4,   // รับข้อมูลจาก Host ทาง EP1 OUT
    HAL_USBDEV_EVT_EP2_OUT     = 5,
    HAL_USBDEV_EVT_EP3_OUT     = 6,
    HAL_USBDEV_EVT_EP4_OUT     = 7,
} hal_usbdev_evt_t;
```

### hal_usbdev_kbd_report_t

```c
typedef struct {
    uint8_t modifiers;  // bit0=LCTRL, bit1=LSHIFT, bit2=LALT, bit3=LGUI
    uint8_t reserved;
    uint8_t keys[6];    // HID usage code ของปุ่มที่กด
} hal_usbdev_kbd_report_t;
```

### hal_usbdev_callback_t

```c
typedef void (*hal_usbdev_callback_t)(hal_usbdev_handle_t h,
                                      hal_usbdev_evt_t evt,
                                      void *user_arg);
```

---

## ฟังก์ชัน

### hal_usbdev_init

```c
hal_usbdev_handle_t hal_usbdev_init(void);
```

**คำอธิบาย:** เริ่มต้น USB Device mode

การทำงานภายใน:
- จัดสรร DMA buffer สำหรับ EP0-EP3 (รวม 576 ไบต์)
- เรียก `USB_DeviceInit()` เพื่อตั้งค่า SIE, เปิด internal pull-up
- ลงทะเบียน descriptor table (Device, Config, HID, String)
- พร้อมตอบสนองต่อ standard request จาก Host ทันที

**ค่าที่คืน:** `hal_usbdev_handle_t` — ถ้าสำเร็จ, คืน handle เดิมถ้าเรียกซ้ำ

**ตัวอย่าง:**
```c
hal_usbdev_handle_t dev = hal_usbdev_init();
```

---

### hal_usbdev_deinit

```c
void hal_usbdev_deinit(hal_usbdev_handle_t h);
```

**คำอธิบาย:** ปิด USB Device mode และล้างสถานะ

---

### hal_usbdev_send

```c
void hal_usbdev_send(hal_usbdev_handle_t h, uint8_t ep,
                     const uint8_t *data, uint8_t len);
```

**คำอธิบาย:** ส่งข้อมูลทาง IN endpoint เมื่อ Host ร้องขอ

**พารามิเตอร์:**

| พารามิเตอร์ | Type | คำอธิบาย |
|---|---|---|
| `h` | `hal_usbdev_handle_t` | handle |
| `ep` | `uint8_t` | เลข endpoint (1-4) |
| `data` | `const uint8_t *` | ข้อมูลที่จะส่ง |
| `len` | `uint8_t` | จำนวนไบต์ (สูงสุด 64) |

**ตัวอย่าง:**
```c
uint8_t report[8] = { 0, 0, 0x04, 0, 0, 0, 0, 0 }; // HID usage 'A'
hal_usbdev_send(dev, 1, report, 8);
```

---

### hal_usbdev_in_ready

```c
uint8_t hal_usbdev_in_ready(hal_usbdev_handle_t h, uint8_t ep);
```

**คำอธิบาย:** ตรวจสอบว่า IN endpoint พร้อมส่งข้อมูลหรือไม่

**ค่าที่คืน:** `1` ถ้าพร้อม (สถานะ NAK), `0` ถ้ายังไม่พร้อม

---

### hal_usbdev_get_recv_data

```c
uint8_t *hal_usbdev_get_recv_data(hal_usbdev_handle_t h, uint8_t ep,
                                  uint8_t *len);
```

**คำอธิบาย:** อ่านข้อมูลที่รับจาก Host ทาง OUT endpoint (ควรเรียกใน callback `HAL_USBDEV_EVT_EPn_OUT`)

**พารามิเตอร์:**

| พารามิเตอร์ | Type | คำอธิบาย |
|---|---|---|
| `h` | `hal_usbdev_handle_t` | handle |
| `ep` | `uint8_t` | เลข endpoint (1-4) |
| `len` | `uint8_t *` | พอยน์เตอร์รับจำนวนไบต์ที่อ่านได้ |

**ค่าที่คืน:** พอยน์เตอร์ไปยัง buffer ที่มีข้อมูล, หรือ `NULL` ถ้า error

---

### hal_usbdev_set_configurated

```c
void hal_usbdev_set_configurated(hal_usbdev_handle_t h);
```

**คำอธิบาย:** ตั้งสถานะ configured (ใช้ใน setup request callback ถ้าต้องการ)

---

### hal_usbdev_attach_cb

```c
void hal_usbdev_attach_cb(hal_usbdev_handle_t h,
                          hal_usbdev_callback_t cb,
                          void *user_arg);
```

**คำอธิบาย:** ลงทะเบียน callback สำหรับเหตุการณ์ USB (reset, suspend, configured, setup req, EP OUT)

**ตัวอย่าง:**
```c
void my_cb(hal_usbdev_handle_t h, hal_usbdev_evt_t evt, void *arg) {
    if (evt == HAL_USBDEV_EVT_RESET) {
        // Reset keyboard state
    } else if (evt == HAL_USBDEV_EVT_CONFIGURED) {
        // Host ตั้งค่าเสร็จ — พร้อมส่ง HID report
    }
}

hal_usbdev_attach_cb(dev, my_cb, NULL);
```

---

### hal_usbdev_int_handler

```c
void hal_usbdev_int_handler(void);
```

**คำอธิบาย:** ฟังก์ชันจัดการ interrupt ของ USB Device — เรียกโดย `USB_IRQHandler` อัตโนมัติ

ภายในจะเรียก `USB_DevTransProcess()` ซึ่งจัดการ standard request และ dispatch event ไปยัง callback

---

## Descriptors ที่ให้มา

| Descriptor | รายละเอียด |
|---|---|
| Device | VID=0x1234, PID=0x0001, class/subclass/protocol = 0 |
| Configuration | 1 interface, 1 HID endpoint IN, 50mA power |
| Interface | HID Boot Keyboard |
| HID | Version 1.10, Report descriptor 63 bytes |
| Endpoint | IN, interrupt, 8 bytes, polling 10ms |
| String | "CH570 HID" |

---

## ข้อควรระวัง

1. **USB Device กับ BLE ใช้ร่วมกันได้** — `USB_DevTransProcess()` ทำงานกับ BLE library
2. **ห้ามใช้ USB Host พร้อมกัน** — ฮาร์ดแวร์ USB ใช้ร่วมกัน
3. **D+ Pull-up** — `hal_usbdev_init()` เปิด internal pull-up ให้อัตโนมัติ
4. **Endpoint buffer** — EP1-EP4 มีขนาด 64 ไบต์ต่อทิศทาง รวม 512B
5. **Enumeration timeout** — ถ้าไม่ตอบสนองต่อ standard request ภายในเวลาที่กำหนด Host จะ disconnect
6. **ปรับแต่ง descriptor** — แก้ไข `hal_usbdev.c` สำหรับ VID/PID/Report descriptor ที่ต้องการ
