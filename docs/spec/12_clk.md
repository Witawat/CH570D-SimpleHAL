# Clock

## Overview

จัดการระบบ clock ของ CH57x — เลือกแหล่ง clock (LSI / HSE / PLL) และตั้งความเร็ว sysclock ได้ตั้งแต่ 1MHz ถึง 100MHz พร้อม config HSE crystal capacitance และ current

## SimpleHAL API

| ฟังก์ชัน | คำอธิบาย |
|---|---|
| `hal_clk_set_sysclock(src)` | ตั้ง sysclock (PLL, HSE, LSI) |
| `hal_clk_get_sysclock()` | อ่านความถี่ปัจจุบัน (Hz) |
| `hal_clk_hse_cfg_cap(pF)` | ตั้งค่า capacitance สำหรับ HSE crystal (18p / 12p / 6p) |
| `hal_clk_hse_cfg_current(mA)` | ตั้งค่า drive current สำหรับ HSE (HALF / FULL) |

**Clock source enum (`hal_clk_sysclock_t`):**

| ค่า | ความถี่ | ต้องใช้ XTAL? |
|---|---|---|
| `HAL_CLK_PLL_100MHz` | 100 MHz | HSE 16MHz |
| `HAL_CLK_PLL_75MHz` | 75 MHz | HSE 12MHz or 8MHz |
| `HAL_CLK_PLL_60MHz` | 60 MHz | HSE 12MHz or 8MHz |
| `HAL_CLK_PLL_50MHz` | 50 MHz | HSE 8MHz |
| `HAL_CLK_PLL_40MHz` | 40 MHz | HSE 8MHz |
| `HAL_CLK_PLL_30MHz` | 30 MHz | HSE 8MHz |
| `HAL_CLK_PLL_25MHz` | 25 MHz | HSE 8MHz |
| `HAL_CLK_PLL_24MHz` | 24 MHz | HSE 8MHz |
| `HAL_CLK_PLL_20MHz` | 20 MHz | HSE 8MHz |
| `HAL_CLK_HSE_16MHz` | 16 MHz | 16MHz XTAL |
| `HAL_CLK_HSE_8MHz` | 8 MHz | 8MHz XTAL |
| `HAL_CLK_HSE_6_4MHz` | 6.4 MHz | 16MHz XTAL + div 2.5 |
| `HAL_CLK_HSE_4MHz` | 4 MHz | 8MHz XTAL + div 2 |
| `HAL_CLK_HSE_2MHz` | 2 MHz | 8MHz XTAL + div 4 |
| `HAL_CLK_HSE_1MHz` | 1 MHz | 8MHz XTAL + div 8 |
| `HAL_CLK_LSI` | ~24 MHz | ใช้ RC ภายใน |

## Circuit Guide

### HSE + PLL (speed สูงสุด — 100MHz)

```
          CH57x
    XTAL_OUT ─────┤├── 16MHz XTAL
    XTAL_IN  ─────┤├──
                    │
              C1(12pF)  C2(12pF)
                    │       │
                   GND     GND

// capacitance ขึ้นกับ spec ของ crystal — ปกติ 12–18pF
// เรียกในโค้ด:
hal_clk_hse_cfg_cap(HSECap_18p);
hal_clk_set_sysclock(HAL_CLK_PLL_100MHz);
```

### LSI (ไม่ต้องใช้ crystal — ประหยัดพิน)

```c
hal_clk_set_sysclock(HAL_CLK_LSI);
// sysclock ≈ 24MHz (ไม่แม่นยำ ±1–2%)
```

## CH57x Specifics

- **PLL input:** ต้องใช้ HSE เป็น source (ไม่สามารถ PLL จาก LSI)
- **HSE crystal:** 1MHz – 16MHz
- **HSE capacitance config:** `HSECap_18p` (default), `HSECap_12p`, `HSECap_6p` — ปรับตาม spec crystal
- **HSE drive current:** `HALF` (default) / `FULL` — FULL สำหรับ crystal ขนาดใหญ่ > 12MHz หรือกินกระแสมาก
- **Peripheral clock:** ต่อจาก sysclock ผ่าน APB bridge — peripheral ทุกตัวทำงานที่ sysclock (ไม่มีการ divide แยก)

## Limitations

| ข้อจำกัด | รายละเอียด |
|---|---|
| เปลี่ยน sysclock ตอน runtime ได้ | แต่ peripheral timer/PWM/UART baud จะเพี้ยน — ควรเปลี่ยนตอน start เท่านั้น |
| PLL lock time | ~1–2µs — รอ `hal_clk_set_sysclock()` return ก่อนใช้ peripheral |
| LSI ไม่แม่นยำพอสำหรับ UART baud > 9600 | ใช้ HSE/PLL สำหรับ UART ที่ต้องการ baud สูง |
| HSE capacitance mismatch | ถ้า capacitance ไม่ตรงกับ crystal → oscillation ไม่ stable หรือกินกระแสเกิน |
| `hal_clk_get_sysclock()` return ค่าประมาณ | อ่านจาก register config (ไม่ใช่วัดจริง) |

## Code สั้น

```c
// ตั้ง 100MHz (ต้องมี 16MHz XTAL)
hal_clk_hse_cfg_cap(HSECap_18p);
hal_clk_set_sysclock(HAL_CLK_PLL_100MHz);

uint32_t freq = hal_clk_get_sysclock();  // 100000000
printf("Sysclock: %lu Hz\r\n", freq);

// หรือ LSI ไม่ต้องใช้ XTAL
// hal_clk_set_sysclock(HAL_CLK_LSI);
```
