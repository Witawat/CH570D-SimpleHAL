# SimpleHAL — USB Host Module

**ไฟล์:** `hal_usbhost.h` / `hal_usbhost.c`

โหมด USB Host — ให้ MCU ควบคุมและสื่อสารกับอุปกรณ์ USB ที่ต่อพ่วง เช่น คีย์บอร์ด เมาส์

**ข้อกำหนดทางฮาร์ดแวร์:**
- ต้องมีแหล่งจ่าย 5V ภายนอกสำหรับ VBUS (MCU จ่าย 3.3V เท่านั้น)
- ต่อ UDP (D+) และ UDM (D-) ของ MCU เข้ากับพอร์ต USB-A female

**การเปิดใช้งาน:** ตั้งค่า `HAL_ENABLE_USBHOST = 1` ใน `simple_hal_config.h`
> ⚠️ ห้ามเปิด `HAL_ENABLE_USBHOST` และ `HAL_ENABLE_USBDEV` พร้อมกัน

---

## รูปแบบข้อมูล

### hal_usbhost_dev_type_t

```c
typedef enum {
    HAL_USBHOST_DEV_NONE      = 0,
    HAL_USBHOST_DEV_KEYBOARD  = 1,
    HAL_USBHOST_DEV_MOUSE     = 2,
    HAL_USBHOST_DEV_HUB       = 3,
    HAL_USBHOST_DEV_STORAGE   = 4,
    HAL_USBHOST_DEV_PRINTER   = 5,
    HAL_USBHOST_DEV_UNKNOWN   = 6,
} hal_usbhost_dev_type_t;
```

### hal_usbhost_evt_t

```c
typedef enum {
    HAL_USBHOST_EVT_NONE        = 0,
    HAL_USBHOST_EVT_CONNECT     = 1,
    HAL_USBHOST_EVT_DISCONNECT  = 2,
    HAL_USBHOST_EVT_READY       = 3,
    HAL_USBHOST_EVT_ERROR       = 4,
} hal_usbhost_evt_t;
```

### hal_usbhost_kbd_report_t

```c
typedef struct {
    uint8_t modifiers;  // bit0=LCTRL, bit1=LSHIFT, bit2=LALT, bit3=LGUI
                        // bit4=RCTRL, bit5=RSHIFT, bit6=RALT, bit7=RGUI
    uint8_t reserved;
    uint8_t keys[6];    // HID usage code ของปุ่มที่กด (สูงสุด 6 ปุ่มพร้อมกัน)
} hal_usbhost_kbd_report_t;
```

### hal_usbhost_callback_t

```c
typedef void (*hal_usbhost_callback_t)(hal_usbhost_handle_t h,
                                       hal_usbhost_evt_t evt,
                                       void *user_arg);
```

---

## ฟังก์ชัน

### hal_usbhost_init

```c
hal_usbhost_handle_t hal_usbhost_init(void);
```

**คำอธิบาย:** เริ่มต้น USB Host mode

การทำงานภายใน:
- เปิด `RB_PIN_USB_EN` ใน `R16_PIN_ALTERNATE` เพื่อเปิดใช้งาน USB analog I/O
- จัดสรร DMA buffer (RX/TX อย่างละ 64 ไบต์)
- เรียก `USB_HostInit()` เพื่อตั้งค่า SIE และเปิด SOF

**ค่าที่คืน:** `hal_usbhost_handle_t` — ถ้าสำเร็จ, คืน handle เดิมถ้าเรียกซ้ำ

**ตัวอย่าง:**
```c
hal_usbhost_handle_t host = hal_usbhost_init();
```

---

### hal_usbhost_deinit

```c
void hal_usbhost_deinit(hal_usbhost_handle_t h);
```

**คำอธิบาย:** ปิด USB Host mode และล้างสถานะ

---

### hal_usbhost_poll

```c
hal_usbhost_evt_t hal_usbhost_poll(hal_usbhost_handle_t h);
```

**คำอธิบาย:** ตรวจสอบเหตุการณ์ USB — ควรเรียกใน main loop ทุก 10-50ms

ตรวจสอบ:
1. การเสียบ/ถอดอุปกรณ์ (ผ่าน `AnalyzeRootHub()`)
2. เมื่อเสียบ → เรียก `InitRootDevice()` เพื่อ enumeration
3. ถ้าเป็น HUB → เรียก `EnumAllHubPort()` เพื่อ enumerate อุปกรณ์ต่อพ่วง

**ค่าที่คืน:** `hal_usbhost_evt_t` — เหตุการณ์ที่เกิดขึ้น

**ตัวอย่าง:**
```c
hal_usbhost_evt_t evt;
while (1) {
    evt = hal_usbhost_poll(host);
    if (evt == HAL_USBHOST_EVT_READY) {
        // อุปกรณ์พร้อมใช้งาน
    } else if (evt == HAL_USBHOST_EVT_DISCONNECT) {
        // อุปกรณ์ถูกถอด
    }
    hal_delay_ms(10);
}
```

---

### hal_usbhost_attach_cb

```c
void hal_usbhost_attach_cb(hal_usbhost_handle_t h,
                           hal_usbhost_callback_t cb,
                           void *user_arg);
```

**คำอธิบาย:** ลงทะเบียน callback สำหรับเหตุการณ์ connect/disconnect/ready

**พารามิเตอร์:**

| พารามิเตอร์ | Type | คำอธิบาย |
|---|---|---|
| `h` | `hal_usbhost_handle_t` | handle จาก `hal_usbhost_init()` |
| `cb` | `hal_usbhost_callback_t` | ฟังก์ชัน callback |
| `user_arg` | `void *` | ข้อมูลส่งกลับไปยัง callback |

**ตัวอย่าง:**
```c
void my_cb(hal_usbhost_handle_t h, hal_usbhost_evt_t evt, void *arg) {
    if (evt == HAL_USBHOST_EVT_READY)
        printf("USB device ready\n");
}

hal_usbhost_attach_cb(host, my_cb, NULL);
```

---

### hal_usbhost_get_device_type

```c
hal_usbhost_dev_type_t hal_usbhost_get_device_type(hal_usbhost_handle_t h);
```

**คำอธิบาย:** คืนประเภทอุปกรณ์ที่ตรวจพบหลัง enumeration

**ค่าที่คืน:** `hal_usbhost_dev_type_t` — หรือ `HAL_USBHOST_DEV_NONE` ถ้ายังไม่มีอุปกรณ์

---

### hal_usbhost_get_vid / hal_usbhost_get_pid

```c
uint16_t hal_usbhost_get_vid(hal_usbhost_handle_t h);
uint16_t hal_usbhost_get_pid(hal_usbhost_handle_t h);
```

**คำอธิบาย:** คืน Vendor ID และ Product ID จาก device descriptor

---

### hal_usbhost_is_ready

```c
uint8_t hal_usbhost_is_ready(hal_usbhost_handle_t h);
```

**คำอธิบาย:** ตรวจสอบว่าอุปกรณ์พร้อมใช้งานหรือไม่

**ค่าที่คืน:** `1` ถ้าพร้อม, `0` ถ้ายังไม่มีอุปกรณ์หรือยังไม่ enumeration เสร็จ

---

### hal_usbhost_kbd_read

```c
hal_status_t hal_usbhost_kbd_read(hal_usbhost_handle_t h,
                                  hal_usbhost_kbd_report_t *report);
```

**คำอธิบาย:** อ่านรายงานคีย์บอร์ด (Interrupt IN transfer) — ใช้ได้เมื่อ device type เป็น `HAL_USBHOST_DEV_KEYBOARD`

**ค่าที่คืน:**

| ค่า | ความหมาย |
|---|---|
| `HAL_OK` | มีข้อมูล — report มี modifier + key codes |
| `HAL_BUSY` | ไม่มีข้อมูลใหม่ (NAK) |
| `HAL_ERROR` | ข้อผิดพลาดในการรับส่งข้อมูล |
| `HAL_INVALID` | handle ไม่ถูกต้อง ไม่ได้ต่อ keyboard หรือ USB Host mode ไม่ได้เปิด |

**ตัวอย่าง:**
```c
hal_usbhost_kbd_report_t report;
if (hal_usbhost_kbd_read(host, &report) == HAL_OK) {
    if (report.modifiers & 0x01) // LCTRL ถูกกด
    if (report.keys[0] == 0x28)  // ปุ่ม 'A' ถูกกด (HID usage code)
}
```

---

## ข้อควรระวัง

1. **VBUS 5V จำเป็น** — MCU ไม่มี regulator ในตัว ต้องใช้ external 5V
2. **Polling-based** — ต้องเรียก `hal_usbhost_poll()` สม่ำเสมอเพื่อตรวจจับการเสียบ/ถอด
3. **Enumeration เป็น blocking (~100ms)** — อาจกระทบ timing ของ BLE/RF
4. **RAM 12KB** — ใช้ DMA buffer 128B + Com_Buffer 128B
5. **USB Host กับ USB Device ใช้ร่วมกันไม่ได้** — ต้องเลือกโหมดใดโหมดหนึ่งใน `simple_hal_config.h`
