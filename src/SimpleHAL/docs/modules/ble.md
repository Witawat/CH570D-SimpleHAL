# SimpleHAL — BLE Module

**ไฟล์:** `hal_ble.h` / `hal_ble.c`

บลูทูธพลังงานต่ำ (BLE) ในโหมด peripheral

**ข้อกำหนด:** ต้องมี BLE library ของ WCH (ไฟล์ `.a`/`.o`) และ include path ไปยัง `BLE/LIB` header — `hal_ble.h` จะ include `CH572BLEPeri_LIB.h` ให้อัตโนมัติ

**การเริ่มต้นภายใน:** `hal_ble_init` จะดำเนินการดังนี้โดยอัตโนมัติ:
- ตรวจสอบเวอร์ชัน header/library (`VER_LIB` vs `VER_FILE`)
- ตั้งค่า SysTick, MISC_CTRL, LLE IRQ handler, และ PFIC priority สำหรับ BLE library
- เรียก `BLE_LibInit` โดยตรง (ไม่ผ่าน wrapper `CH57x_BLEInit`)
- ลงทะเบียน TMOS task และเริ่ม GAP Peripheral Role

---

## ฟังก์ชัน

### hal_ble_init

```c
hal_ble_handle_t hal_ble_init(const char *device_name, uint8_t tx_power);
```

**คำอธิบาย:** เริ่มต้น BLE peripheral

**พารามิเตอร์:**

| พารามิเตอร์ | Type | คำอธิบาย | ค่าที่เป็นไปได้ |
|---|---|---|---|
| `device_name` | `const char *` | ชื่ออุปกรณ์สำหรับ advertising | สูงสุด 15 ตัวอักษร |
| `tx_power` | `uint8_t` | กำลังส่ง | `LL_TX_POWEER_0_DBM`, `LL_TX_POWEER_4_DBM`, `LL_TX_POWEER_NEG12_DBM`, ฯลฯ (ดูใน BLE library) |

**ค่าที่คืน:** `hal_ble_handle_t` — ถ้าสำเร็จ, คืน handle เดิมถ้าเรียกซ้ำ (ไม่เกิดการ init ซ้ำ)

**ตัวอย่าง:**

```c
hal_ble_handle_t ble = hal_ble_init("nanoCH572", LL_TX_POWEER_0_DBM);
```

---

### hal_ble_advertise_start

```c
void hal_ble_advertise_start(hal_ble_handle_t h, hal_ble_adv_mode_t mode);
```

**คำอธิบาย:** เริ่มประกาศตัว (advertising) เพื่อให้อุปกรณ์อื่นค้นพบ

**พารามิเตอร์:**

| พารามิเตอร์ | Type | คำอธิบาย | ค่าที่เป็นไปได้ |
|---|---|---|---|
| `h` | `hal_ble_handle_t` | handle BLE | — |
| `mode` | `hal_ble_adv_mode_t` | โหมด advertising | `HAL_BLE_ADV_MODE_CONNECTABLE`, `HAL_BLE_ADV_MODE_NON_CONNECTABLE` |

**โหมด advertising (`hal_ble_adv_mode_t`):**

| ค่า | คำอธิบาย |
|---|---|
| `HAL_BLE_ADV_MODE_CONNECTABLE` | ประกาศตัวแบบให้เชื่อมต่อได้ |
| `HAL_BLE_ADV_MODE_NON_CONNECTABLE` | ประกาศตัวแบบไม่รับการเชื่อมต่อ (broadcast only) |

```c
hal_ble_advertise_start(ble, HAL_BLE_ADV_MODE_CONNECTABLE);
```

---

### hal_ble_advertise_stop

```c
void hal_ble_advertise_stop(hal_ble_handle_t h);
```

**คำอธิบาย:** หยุดประกาศตัว

---

### hal_ble_send

```c
hal_status_t hal_ble_send(hal_ble_handle_t h, const uint8_t *data, uint16_t len);
```

**คำอธิบาย:** ส่งข้อมูลไปยังอุปกรณ์ที่เชื่อมต่ออยู่

**พารามิเตอร์:**

| พารามิเตอร์ | Type | คำอธิบาย |
|---|---|---|
| `h` | `hal_ble_handle_t` | handle BLE |
| `data` | `const uint8_t *` | ข้อมูลที่จะส่ง |
| `len` | `uint16_t` | จำนวนไบต์ (สูงสุด 20 ไบต์สำหรับ BLE notification) |

**ค่าที่คืน:** `HAL_OK` ถ้าสำเร็จ, `HAL_ERROR` ถ้าไม่ได้เชื่อมต่อ

```c
uint8_t msg[] = "Hello BLE!";
hal_ble_send(ble, msg, sizeof(msg));
```

---

### hal_ble_attach_connect_cb

```c
void hal_ble_attach_connect_cb(hal_ble_handle_t h, hal_callback_t cb, void *arg);
```

**คำอธิบาย:** แนบ callback function เมื่อมีอุปกรณ์มาเชื่อมต่อ

**พารามิเตอร์:**

| พารามิเตอร์ | Type | คำอธิบาย |
|---|---|---|
| `h` | `hal_ble_handle_t` | handle BLE |
| `cb` | `hal_callback_t` | ฟังก์ชัน callback |
| `arg` | `void *` | อาร์กิวเมนต์ส่งไปยัง callback |

---

### hal_ble_attach_disconnect_cb

```c
void hal_ble_attach_disconnect_cb(hal_ble_handle_t h, hal_callback_t cb, void *arg);
```

**คำอธิบาย:** แนบ callback function เมื่อขาดการเชื่อมต่อ

**พารามิเตอร์:**

| พารามิเตอร์ | Type | คำอธิบาย |
|---|---|---|
| `h` | `hal_ble_handle_t` | handle BLE |
| `cb` | `hal_callback_t` | ฟังก์ชัน callback |
| `arg` | `void *` | อาร์กิวเมนต์ส่งไปยัง callback |

---

### hal_ble_attach_data_cb

```c
void hal_ble_attach_data_cb(hal_ble_handle_t h, hal_callback_t cb, void *arg);
```

**คำอธิบาย:** แนบ callback function เมื่อได้รับข้อมูลจากอุปกรณ์ที่เชื่อมต่อ

**พารามิเตอร์:**

| พารามิเตอร์ | Type | คำอธิบาย |
|---|---|---|
| `h` | `hal_ble_handle_t` | handle BLE |
| `cb` | `hal_callback_t` | ฟังก์ชัน callback |
| `arg` | `void *` | อาร์กิวเมนต์ส่งไปยัง callback |

```c
void on_ble_data(void *arg) {
    hal_ble_handle_t ble = (hal_ble_handle_t)arg;
    // อ่านข้อมูล
}
hal_ble_attach_data_cb(ble, on_ble_data, ble);
```

---

### hal_ble_is_connected

```c
uint8_t hal_ble_is_connected(hal_ble_handle_t h);
```

**คำอธิบาย:** ตรวจสอบสถานะการเชื่อมต่อ

**พารามิเตอร์:**

| พารามิเตอร์ | Type | คำอธิบาย |
|---|---|---|
| `h` | `hal_ble_handle_t` | handle BLE |

**ค่าที่คืน:** `1` ถ้าเชื่อมต่ออยู่, `0` ถ้าไม่ได้เชื่อมต่อ

```c
if (hal_ble_is_connected(ble)) {
    hal_ble_send(ble, data, len);
}
```

---

### hal_ble_process

```c
void hal_ble_process(void);
```

**คำอธิบาย:** ประมวลผล BLE stack — **ต้องเรียกฟังก์ชันนี้ใน loop หลักเป็นระยะ** เพื่อให้ BLE stack ทำงาน

```c
while (1) {
    hal_ble_process();
    // โค้ดอื่นๆ
}
```
