# SimpleHAL — Power Module

**ไฟล์:** `hal_pwr.h` / `hal_pwr.c`

จัดการโหมดประหยัดพลังงานของ CH572

---

## ฟังก์ชัน

### hal_pwr_idle

```c
void hal_pwr_idle(void);
```

**คำอธิบาย:** โหมด Idle — หยุด CPU แต่ peripheral ยังทำงานได้ ปลุกด้วย interrupt ใดๆ

**การปลุก:** interrupt ใดๆ ก็ตาม

| ลักษณะ | รายละเอียด |
|---|---|
| CPU | หยุด |
| Peripheral | ทำงานปกติ |
| ประหยัดไฟ | น้อย |
| ปลุก | interrupt ใดๆ |

```c
hal_pwr_idle();
// เมื่อมี interrupt CPU จะทำงานต่อจากบรรทัดนี้
```

---

### hal_pwr_halt

```c
void hal_pwr_halt(void);
```

**คำอธิบาย:** โหมด Halt — หยุด CPU และ peripheral, ปลุกด้วย interrupt ภายนอกเท่านั้น

| ลักษณะ | รายละเอียด |
|---|---|
| CPU | หยุด |
| Peripheral | หยุด |
| ประหยัดไฟ | ปานกลาง |
| ปลุก | interrupt ภายนอก (GPIO, ฯลฯ) |

---

### hal_pwr_sleep

```c
void hal_pwr_sleep(uint16_t retention_mask);
```

**คำอธิบาย:** โหมด Sleep — ประหยัดไฟสูง ต้องตั้งค่า wakeup source ก่อนด้วย `hal_pwr_wakeup_cfg()`

**พารามิเตอร์:**

| พารามิเตอร์ | Type | คำอธิบาย | ค่าที่เป็นไปได้ |
|---|---|---|---|
| `retention_mask` | `uint16_t` | mask สำหรับคงสถานะ RAM/peripheral | `RB_PWR_RAM2K`, `RB_PWR_RAM16K`, `RB_PWR_EXTEND`, `RB_PWR_XROM`, หรือ `0` (ไม่คง) |

**ค่า `retention_mask`:**

| ค่า | คำอธิบาย |
|---|---|
| `RB_PWR_RAM2K` | คง RAM 2KB |
| `RB_PWR_RAM16K` | คง RAM 16KB |
| `RB_PWR_EXTEND` | คง USB/BLE |
| `RB_PWR_XROM` | คง Flash |
| `0` | ไม่คงอะไรเลย |

```c
hal_pwr_wakeup_cfg(RB_SLP_RTC_WAKE, HAL_PWR_WAKE_DELAY_64);
hal_pwr_sleep(RB_PWR_RAM2K);
```

---

### hal_pwr_shutdown

```c
void hal_pwr_shutdown(uint16_t retention_mask);
```

**คำอธิบาย:** โหมด Shutdown — ประหยัดไฟสูงสุด ต้องรีเซ็ตระบบเพื่อปลุก (ไม่สามารถปลุกด้วย interrupt)

**พารามิเตอร์:**

| พารามิเตอร์ | Type | คำอธิบาย |
|---|---|---|
| `retention_mask` | `uint16_t` | mask สำหรับคงสถานะ (เหมือน `hal_pwr_sleep`) |

---

### hal_pwr_wakeup_cfg

```c
void hal_pwr_wakeup_cfg(uint8_t source, hal_pwr_wake_delay_t delay);
```

**คำอธิบาย:** ตั้งค่าแหล่งปลุกและดีเลย์สำหรับโหมด sleep

**พารามิเตอร์:**

| พารามิเตอร์ | Type | คำอธิบาย | ค่าที่เป็นไปได้ |
|---|---|---|---|
| `source` | `uint8_t` | แหล่งสัญญาณปลุก | `RB_SLP_USB_WAKE`, `RB_SLP_RTC_WAKE`, `RB_SLP_GPIO_WAKE`, `RB_SLP_BAT_WAKE` |
| `delay` | `hal_pwr_wake_delay_t` | ดีเลย์หลังปลุกก่อน CPU เริ่มทำงาน | ดูตารางด้านล่าง |

**ดีเลย์ (`hal_pwr_wake_delay_t`):**

| ค่า | ดีเลย์ |
|---|---|
| `HAL_PWR_WAKE_DELAY_1` | 1 cycle |
| `HAL_PWR_WAKE_DELAY_64` | 64 cycles |
| `HAL_PWR_WAKE_DELAY_512` | 512 cycles |
| `HAL_PWR_WAKE_DELAY_3584` | 3584 cycles |
| `HAL_PWR_WAKE_DELAY_4096` | 4096 cycles |
| `HAL_PWR_WAKE_DELAY_6144` | 6144 cycles |
| `HAL_PWR_WAKE_DELAY_7168` | 7168 cycles |
| `HAL_PWR_WAKE_DELAY_8191` | 8191 cycles |

```c
hal_pwr_wakeup_cfg(RB_SLP_RTC_WAKE | RB_SLP_GPIO_WAKE, HAL_PWR_WAKE_DELAY_64);
```

---

### hal_pwr_periph_clk

```c
void hal_pwr_periph_clk(uint8_t enable, uint16_t periph_mask);
```

**คำอธิบาย:** เปิด/ปิดนาฬิกาของ peripheral เพื่อประหยัดไฟขณะรัน

**พารามิเตอร์:**

| พารามิเตอร์ | Type | คำอธิบาย | ค่าที่เป็นไปได้ |
|---|---|---|---|
| `enable` | `uint8_t` | เปิดหรือปิด | `0` = ปิด, `1` = เปิด |
| `periph_mask` | `uint16_t` | peripheral ที่ต้องการควบคุม | `RB_CLK_USB`, `RB_CLK_SPI`, `RB_CLK_UART`, `RB_CLK_I2C`, ฯลฯ |

```c
hal_pwr_periph_clk(0, RB_CLK_USB);  // ปิด USB clock
hal_pwr_periph_clk(1, RB_CLK_SPI);  // เปิด SPI clock
```
