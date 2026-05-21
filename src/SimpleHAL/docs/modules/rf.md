# SimpleHAL — RF Module

**ไฟล์:** `hal_rf.h` / `hal_rf.c`

สื่อสาร 2.4GHz ไร้สาย (ใช้ RF core ของ CH572)

---

## ฟังก์ชัน

### hal_rf_init

```c
hal_rf_handle_t hal_rf_init(uint32_t frequency_khz, hal_rf_phy_mode_t phy, int8_t tx_power_dbm);
```

**คำอธิบาย:** เริ่มต้น RF 2.4GHz

**พารามิเตอร์:**

| พารามิเตอร์ | Type | คำอธิบาย | ค่าที่เป็นไปได้ |
|---|---|---|---|
| `frequency_khz` | `uint32_t` | ความถี่ในหน่วย kHz | เช่น `2402000` = 2.402 GHz |
| `phy` | `hal_rf_phy_mode_t` | โหมด PHY | `HAL_RF_PHY_1M`, `HAL_RF_PHY_2M`, `HAL_RF_PHY_2G4` |
| `tx_power_dbm` | `int8_t` | กำลังส่ง (dBm) | สูงสุด +7 dBm (เช่น `0`, `4`, `7`) |

**โหมด PHY (`hal_rf_phy_mode_t`):**

| ค่า | อัตรา | คำอธิบาย |
|---|---|---|
| `HAL_RF_PHY_1M` | 1 Mbps | 1Mbps โหมด |
| `HAL_RF_PHY_2M` | 2 Mbps | 2Mbps Enhanced ShockBurst |
| `HAL_RF_PHY_2G4` | 2 Mbps | 2.4Gbps Proprietary (nRF24L01 compatible) |

**ค่าที่คืน:** `hal_rf_handle_t` — ถ้าสำเร็จ, `NULL` ถ้าไม่สำเร็จ

**ตัวอย่าง:**

```c
hal_rf_handle_t rf = hal_rf_init(2402000, HAL_RF_PHY_2G4, 0);
```

---

### hal_rf_send

```c
hal_status_t hal_rf_send(hal_rf_handle_t h, const uint8_t *data, uint16_t len);
```

**คำอธิบาย:** ส่งข้อมูลไร้สายแบบบล็อก

**พารามิเตอร์:**

| พารามิเตอร์ | Type | คำอธิบาย |
|---|---|---|
| `h` | `hal_rf_handle_t` | handle RF |
| `data` | `const uint8_t *` | ข้อมูลที่จะส่ง |
| `len` | `uint16_t` | จำนวนไบต์ (สูงสุด 251 ไบต์) |

**ค่าที่คืน:** `HAL_OK` ถ้าสำเร็จ, `HAL_ERROR` ถ้าส่งล้มเหลว

```c
uint8_t msg[] = "Hello RF!";
hal_rf_send(rf, msg, sizeof(msg));
```

---

### hal_rf_recv

```c
hal_status_t hal_rf_recv(hal_rf_handle_t h, uint8_t *buf, uint16_t max_len, uint16_t *out_len, int8_t *rssi, uint32_t timeout_us);
```

**คำอธิบาย:** รับข้อมูลไร้สายแบบบล็อก (รอจนกว่าจะมีข้อมูลหรือหมดเวลา)

**พารามิเตอร์:**

| พารามิเตอร์ | Type | คำอธิบาย |
|---|---|---|
| `h` | `hal_rf_handle_t` | handle RF |
| `buf` | `uint8_t *` | บัฟเฟอร์รับข้อมูล |
| `max_len` | `uint16_t` | ขนาดสูงสุดของบัฟเฟอร์ |
| `out_len` | `uint16_t *` | ตัวแปรรับจำนวนไบต์ที่รับได้ (หรือ `NULL`) |
| `rssi` | `int8_t *` | ตัวแปรรับค่าความแรงสัญญาณใน dBm (หรือ `NULL`) |
| `timeout_us` | `uint32_t` | ระยะเวลารอในหน่วย µs (`0` = รอตลอดไป) |

**ค่าที่คืน:** `HAL_OK` ถ้าได้รับข้อมูล, `HAL_TIMEOUT` ถ้าหมดเวลา

```c
uint8_t buf[32];
uint16_t len;
int8_t rssi;
if (hal_rf_recv(rf, buf, sizeof(buf), &len, &rssi, 1000000) == HAL_OK) {
    // ได้ข้อมูล len ไบต์ สัญญาณแรง rssi dBm
}
```

---

### hal_rf_set_tx_power

```c
void hal_rf_set_tx_power(hal_rf_handle_t h, int8_t power_dbm);
```

**คำอธิบาย:** ปรับกำลังส่ง

**พารามิเตอร์:**

| พารามิเตอร์ | Type | คำอธิบาย | ค่าที่เป็นไปได้ |
|---|---|---|---|
| `h` | `hal_rf_handle_t` | handle RF | — |
| `power_dbm` | `int8_t` | กำลังส่งใน dBm | `-25` ถึง `+7` |

---

### hal_rf_attach_rx_cb

```c
void hal_rf_attach_rx_cb(hal_rf_handle_t h, hal_callback_t cb, void *arg);
```

**คำอธิบาย:** แนบ callback function เมื่อมีข้อมูลขาเข้า (ต้องเรียก `RFIP_SetRx` ก่อนเพื่อเปิดรับ)

**พารามิเตอร์:**

| พารามิเตอร์ | Type | คำอธิบาย |
|---|---|---|
| `h` | `hal_rf_handle_t` | handle RF |
| `cb` | `hal_callback_t` | ฟังก์ชัน callback |
| `arg` | `void *` | อาร์กิวเมนต์ส่งไปยัง callback |

---

### hal_rf_stop

```c
void hal_rf_stop(hal_rf_handle_t h);
```

**คำอธิบาย:** หยุดและปิด RF

---

### hal_rf_sleep

```c
void hal_rf_sleep(hal_rf_handle_t h);
```

**คำอธิบาย:** ปิด RF เพื่อประหยัดไฟ (สามารถปลุกได้ด้วย `hal_rf_wake`)

---

### hal_rf_wake

```c
void hal_rf_wake(hal_rf_handle_t h);
```

**คำอธิบาย:** ปลุก RF จากโหมด sleep

---

### hal_rf_set_channel

```c
void hal_rf_set_channel(hal_rf_handle_t h, uint32_t frequency_khz);
```

**คำอธิบาย:** เปลี่ยนช่องความถี่ขณะรัน

**พารามิเตอร์:**

| พารามิเตอร์ | Type | คำอธิบาย |
|---|---|---|
| `h` | `hal_rf_handle_t` | handle RF |
| `frequency_khz` | `uint32_t` | ความถี่ใหม่ (kHz) |

---

### hal_rf_calibrate

```c
void hal_rf_calibrate(hal_rf_handle_t h);
```

**คำอธิบาย:** ปรับเทียบ RF — ควรเรียกหลังจากเปลี่ยนอุณหภูมิมากๆ เพื่อรักษาประสิทธิภาพ
