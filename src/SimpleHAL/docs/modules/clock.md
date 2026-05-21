# SimpleHAL — Clock Module

**ไฟล์:** `hal_clk.h` / `hal_clk.c`

ตั้งค่าและอ่านความถี่นาฬิการะบบ และปรับแต่ง HSE oscillator

---

## ฟังก์ชัน

### hal_clk_set_sysclock

```c
void hal_clk_set_sysclock(hal_sysclk_t clk);
```

**คำอธิบาย:** ตั้งความถี่นาฬิการะบบ

**พารามิเตอร์:**

| พารามิเตอร์ | Type | คำอธิบาย | ค่าที่เป็นไปได้ |
|---|---|---|---|
| `clk` | `hal_sysclk_t` | ความถี่เป้าหมาย | ดูตารางด้านล่าง |

**ค่าที่เป็นไปได้ (`hal_sysclk_t`):**

| ค่า | ความถี่ | แหล่ง |
|---|---|---|
| `HAL_CLK_LSI` | ~32 kHz | LSI internal |
| `HAL_CLK_HSE_4MHz` | 4 MHz | HSE crystal |
| `HAL_CLK_HSE_8MHz` | 8 MHz | HSE crystal |
| `HAL_CLK_HSE_16MHz` | 16 MHz | HSE crystal (direct) |
| `HAL_CLK_PLL_20MHz` | 20 MHz | HSE + PLL |
| `HAL_CLK_PLL_24MHz` | 24 MHz | HSE + PLL |
| `HAL_CLK_PLL_25MHz` | 25 MHz | HSE + PLL |
| `HAL_CLK_PLL_30MHz` | 30 MHz | HSE + PLL |
| `HAL_CLK_PLL_40MHz` | 40 MHz | HSE + PLL |
| `HAL_CLK_PLL_50MHz` | 50 MHz | HSE + PLL |
| `HAL_CLK_PLL_60MHz` | 60 MHz | HSE + PLL |
| `HAL_CLK_PLL_75MHz` | 75 MHz | HSE + PLL |
| `HAL_CLK_PLL_100MHz` | 100 MHz | HSE + PLL |

**ตัวอย่าง:**

```c
hal_clk_set_sysclock(HAL_CLK_PLL_100MHz);  // 100 MHz
hal_clk_set_sysclock(HAL_CLK_PLL_60MHz);   // 60 MHz
hal_clk_set_sysclock(HAL_CLK_HSE_16MHz);   // 16 MHz (ตรง)

uint32_t freq = hal_clk_get_sysclock();    // อ่านค่าปัจจุบัน
```

---

### hal_clk_get_sysclock

```c
uint32_t hal_clk_get_sysclock(void);
```

**คำอธิบาย:** อ่านความถี่นาฬิการะบบปัจจุบัน

**ค่าที่คืน:** ความถี่ในหน่วย Hz (เช่น `100000000` สำหรับ 100MHz)

```c
uint32_t sysclk = hal_clk_get_sysclock();
```

---

### hal_clk_hse_cfg_cap

```c
void hal_clk_hse_cfg_cap(uint8_t cap);
```

**คำอธิบาย:** ตั้งค่าความจุโหลดของ HSE oscillator ให้ตรงกับ crystal ที่ใช้

**พารามิเตอร์:**

| พารามิเตอร์ | Type | คำอธิบาย | ค่าที่เป็นไปได้ |
|---|---|---|---|
| `cap` | `uint8_t` | ค่าความจุโหลด | `0`–`7` (6pF–20pF โดยประมาณ), หรือใช้ macro `HSECap_18p`, `HSECap_12p`, ฯลฯ |

```c
hal_clk_hse_cfg_cap(HSECap_18p);  // 18pF
```

---

### hal_clk_hse_cfg_current

```c
void hal_clk_hse_cfg_current(uint8_t cur);
```

**คำอธิบาย:** ตั้งค่ากระแสไบอัสของ HSE oscillator

**พารามิเตอร์:**

| พารามิเตอร์ | Type | คำอธิบาย | ค่าที่เป็นไปได้ |
|---|---|---|---|
| `cur` | `uint8_t` | ระดับกระแสไบอัส | `0` (75%), `1`, `2`, `3` (150%) |

```c
hal_clk_hse_cfg_current(0);  // 75%
```
