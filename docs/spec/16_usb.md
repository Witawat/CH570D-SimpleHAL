# USB

## Overview

CH57x มี USB 2.0 full-speed (12Mbps) — Device mode ในทุกรุ่น, Host mode เฉพาะ CH571/CH572 เป็น optional module ที่ต้องเปิดด้วย `HAL_ENABLE_USBDEV=1` หรือ `HAL_ENABLE_USBHOST=1`

## SimpleHAL API

### USB Device

| ฟังก์ชัน | คำอธิบาย |
|---|---|
| `hal_usbdev_init()` | เริ่มต้น USB Device |
| `hal_usbdev_deinit()` | หยุด USB Device |
| `hal_usbdev_send(ep, data, len)` | ส่งข้อมูลผ่าน endpoint |
| `hal_usbdev_in_ready(ep)` | เช็คว่า endpoint IN พร้อมส่ง |
| `hal_usbdev_get_recv_data(ep, buf)` | อ่านข้อมูลที่รับมา |
| `hal_usbdev_attach_cb(event, cb)` | ตั้ง callback สำหรับ event |

### USB Host

| ฟังก์ชัน | คำอธิบาย |
|---|---|
| `hal_usbhost_init()` | เริ่มต้น USB Host |
| `hal_usbhost_poll()` | เรียกใน main loop เพื่อจัดการ host |
| `hal_usbhost_get_device_type()` | คืนประเภท device ที่ต่อ |
| `hal_usbhost_get_vid()` | อ่าน Vendor ID |
| `hal_usbhost_get_pid()` | อ่าน Product ID |
| `hal_usbhost_is_ready()` | เช็คว่า device พร้อม |
| `hal_usbhost_kbd_read(buf)` | อ่าน keyboard report (HID) |
| `hal_usbhost_attach_cb(event, cb)` | ตั้ง callback สำหรับ event |

**USB Device events:** `DEV_CONNECTED`, `DEV_DISCONNECTED`, `DEV_SUSPEND`, `DEV_CONFIGURED`, `DEV_SETUP`, `DEV_EP_OUT`

**USB Host events:** `HOST_CONNECT`, `HOST_DISCONNECT`, `HOST_READY`, `HOST_ERROR`

## Circuit Guide

### USB Device

```
CH57x                      USB Port (Type-C / Micro-USB)
  USB_DM (PA11) ────────── DM (D-)
  USB_DP (PA12) ────────── DP (D+)
  GND ──────────────────── GND
  VCC ──────────────────── VBUS (optional — for detection)

// PA11/PA12 เป็นพินตายตัวสำหรับ USB
```

### USB Host

```
CH57x (CH571/CH572)       USB Device (Keyboard/Mouse/etc.)
  USB_DM (PA11) ────────── DM
  USB_DP (PA12) ────────── DP
  VBUS_EN (GPIO) ──────── VCC (5V — ต้องใช้ external switch หรือ regulator)
  GND ──────────────────── GND

// VBUS ต้อง power จากภายนอก (5V at 500mA) — CH57x ไม่มี internal boost
```

## CH57x Specifics

- **Speed:** USB 2.0 Full-Speed (12Mbps)
- **Endpoints (Device):** 4 endpoints (EP0 control + EP1–EP4 data)
- **Buffer:** 64 bytes per endpoint
- **Transfer types:** Control, Interrupt, Bulk
- **HID:** Hardware support สำหรับ HID report descriptor
- **Root hub (Host):** 1 port ต่อ device ได้ 1 ตัว (ยกเว้นมี external HUB)
- **Dual USB (CH572):** รองรับการต่อ USB Device + Host พร้อมกัน (2 USB controller) — ต้องใช้ `USB2` pins

## Limitations

| ข้อจำกัด | รายละเอียด |
|---|---|
| USB Host = CH571/CH572 only | CH570 ไม่มี USB Host |
| Host รองรับแค่ HID keyboard/mouse + HUB + storage | ไม่ใช่ generic Host — ไม่ support CDC หรือ printer class |
| Device รองรับแค่ HID keyboard | SimpleHAL usbdev ส่ง keyboard report (8 bytes) — ถ้าต้องการ custom HID ใช้ StdPeriphDriver โดยตรง |
| 64 bytes buffer ต่อ endpoint | ไม่เหมาะกับ bulk transfer ขนาดใหญ่ |
| VBUS ต้อง supply ภายนอก | Host mode = ต้องมี 5V regulator แยก (AP1501, LM2596, หรือ USB port power) |
| USB 3.0 ไม่รองรับ | Full-speed (12Mbps) only |

## Code สั้น

```c
// USB HID Keyboard (Device)
hal_usbdev_init();
hal_usbdev_attach_cb(DEV_CONFIGURED, []() {
    printf("USB configured!\r\n");
});

// ส่ง key press (A)
uint8_t report[8] = {0, 0, 4, 0, 0, 0, 0, 0}; // modifier, reserved, keycode...
hal_usbdev_send(1, report, 8);

// USB Host Keyboard
hal_usbhost_init();
while (1) {
    hal_usbhost_poll();
    if (hal_usbhost_is_ready()) {
        uint8_t buf[8];
        hal_usbhost_kbd_read(buf);
        printf("Key: %02X %02X\r\n", buf[0], buf[2]);
    }
    Delay_Ms(10);
}
```
