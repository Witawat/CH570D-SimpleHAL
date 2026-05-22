# HX711 Load Cell ADC Library

> **Library สำหรับอ่านน้ำหนักจาก Load Cell ผ่าน HX711 ADC บน CH57x**

## 📋 สารบัญ

1. [ภาพรวม](#ภาพรวม)
2. [คุณสมบัติ](#คุณสมบัติ)
3. [Hardware Setup](#hardware-setup)
4. [หลักการทำงาน](#หลักการทำงาน)
5. [การใช้งานพื้นฐาน](#การใช้งานพื้นฐาน)
6. [Calibration](#calibration)
7. [การใช้งานขั้นสูง](#การใช้งานขั้นสูง)
8. [Troubleshooting](#troubleshooting)
9. [API Reference](#api-reference)

---

## ภาพรวม

HX711 เป็น **24-bit ADC** ที่ออกแบบมาเฉพาะสำหรับ **Load Cell** และ **Wheatstone Bridge**  
ใช้โปรโตคอลสื่อสาร 2 สาย (DOUT + SCK) ที่ไม่ใช่ I2C หรือ SPI

**คุณสมบัติของ HX711:**
- ADC 24-bit (ค่า raw: -8388608 ถึง 8388607)
- 2 ช่อง input: Channel A (Gain 128 หรือ 64), Channel B (Gain 32)
- Output rate: 10Hz หรือ 80Hz (เลือกได้ผ่าน RATE pin)
- ไฟเลี้ยง: 2.6V–5.5V
- มี Power Down mode

---

## คุณสมบัติของ Library

- ✅ อ่านค่า raw 24-bit
- ✅ Tare (ตั้งศูนย์) ด้วยการเฉลี่ยหลาย samples
- ✅ Calibration factor (แปลงค่า raw → กรัม)
- ✅ อ่านน้ำหนักเฉลี่ยหลาย samples
- ✅ เลือก Gain: 128, 64, 32
- ✅ Power Down / Power Up
- ✅ ตรวจสอบ ready state
- ✅ เอกสารภาษาไทยครบถ้วน

---

## Hardware Setup

### วงจร Load Cell + HX711

```
Load Cell (4 สาย)          HX711 Module
                            +----------+
สีแดง   (E+) -----------> | E+  VCC  | <------ 3.3V
สีดำ    (E-) -----------> | E-  GND  | -------> GND
สีขาว   (A-) -----------> | A-  DOUT | -------> GPIO (เช่น PD4)
สีเขียว (A+) -----------> | A+  SCK  | <------- GPIO (เช่น PD3)
                            |    RATE  | --- GND (10Hz) หรือ VCC (80Hz)
                            +----------+
```

```
CH57x           HX711 Module
PD4 -----------> DOUT  (Data Output)
PD3 -----------> SCK   (Clock Input)
3.3V ----------> VCC
GND -----------> GND
```

> ℹ️ ไม่ต้องการ pull-up resistor สำหรับ HX711

### โครงสร้าง Load Cell (Wheatstone Bridge)

```
                    VCC (Excitation+)
                         |
            +------------+------------+
            |                         |
           [R1]                      [R3]
            |                         |
           A+ ──── Amplifier ──── A-
            |                         |
           [R2]                      [R4]
            |                         |
            +------------+------------+
                         |
                    GND (Excitation-)

เมื่อมีแรงกด:
  R1, R4 → เพิ่ม (tension)
  R2, R3 → ลด (compression)
  → เกิดแรงดันต่างระหว่าง A+ และ A-
  → HX711 ขยายและ convert เป็น 24-bit
```

---

## หลักการทำงาน

### โปรโตคอล HX711

```
DOUT: HIGH ─────────────────────────────────────────────────── (busy)
          │
          └─ LOW (ready signal)

SCK:   ____┌─┐ ┌─┐ ┌─┐ ... ┌─┐ ┌─┐ ┌─┐    ← 24+n pulses
            │ │ │ │ │ │     │ │ │ │ │ │
                                           n = 1 → Gain 128 (CH-A)
                                           n = 2 → Gain 32  (CH-B)
                                           n = 3 → Gain 64  (CH-A)

อ่านข้อมูล: ทุก rising edge ของ SCK → ตรวจ DOUT
MSB ส่งก่อน (bit 23 → bit 0)
```

### Sign Extension

ค่า raw ของ HX711 เป็น two's complement 24-bit:
```
bit 23 = sign bit
  0xxxxxxxxxxxxxxxxxxxxxxx → บวก (0 ถึง 8,388,607)
  1xxxxxxxxxxxxxxxxxxxxxxx → ลบ  (-8,388,608 ถึง -1)

Library แปลงเป็น int32_t โดย sign extend:
  ถ้า bit23=1 → OR ด้วย 0xFF000000
```

---

## การใช้งานพื้นฐาน

### ขั้นตอนที่ 1: Include และประกาศตัวแปร

```c
#include "SimpleHAL.h"
#include "HX711.h"

HX711_Instance scale;
```

### ขั้นตอนที่ 2: Init ใน main()

```c
int main(void) {
    SystemCoreClockUpdate();
    Timer_Init();

    // DOUT=PD4, SCK=PD3
    HX711_Init(&scale, PIN_PD4, PIN_PD3);
```

### ขั้นตอนที่ 3: Tare (ตั้งศูนย์)

```c
    printf("กรุณาเอาของออกจาก scale แล้ว กด tare...\r\n");
    Delay_Ms(2000);

    HX711_Tare(&scale, 10);  // เฉลี่ย 10 ครั้ง
    printf("Tare เสร็จแล้ว\r\n");
```

### ขั้นตอนที่ 4: อ่านน้ำหนัก

```c
    while (1) {
        float weight;
        HX711_GetWeight(&scale, &weight);
        printf("น้ำหนัก: %.1f g\r\n", weight);
        Delay_Ms(500);
    }
}
```

---

## Calibration

Calibration ต้องทำ **ครั้งแรก** หรือเมื่อเปลี่ยน hardware

### วิธี Calibration แบบ manual

```c
// ขั้นตอน:
// 1. Tare (ไม่มีน้ำหนักบน scale)
HX711_Tare(&scale, 10);

// 2. วางน้ำหนักที่รู้ค่า เช่น 500g
printf("วาง 500g บน scale แล้วรอ 5 วินาที...\r\n");
Delay_Ms(5000);

// 3. อ่านค่า raw (หลัง tare แล้ว raw = raw_total - tare_offset)
int32_t raw;
HX711_Read(&scale, &raw);
int32_t raw_net = raw - scale.tare_offset;

printf("Raw value (500g): %ld\r\n", raw_net);

// 4. คำนวณและตั้ง calibration factor
float factor = (float)raw_net / 500.0f;
HX711_SetCalibration(&scale, factor);

printf("Calibration factor: %.2f\r\n", factor);
// บันทึกค่านี้ลง EEPROM เพื่อใช้ครั้งต่อไป
```

### บันทึก Calibration ลง EEPROM (AT24C02)

```c
#include "AT24Cxx.h"

AT24Cxx_Instance eeprom;

void save_calibration(void) {
    AT24Cxx_WriteArray(&eeprom, 0x00, (uint8_t*)&scale.calibration_factor, sizeof(float));
    AT24Cxx_WriteArray(&eeprom, 0x04, (uint8_t*)&scale.tare_offset, sizeof(int32_t));
}

void load_calibration(void) {
    AT24Cxx_ReadArray(&eeprom, 0x00, (uint8_t*)&scale.calibration_factor, sizeof(float));
    AT24Cxx_ReadArray(&eeprom, 0x04, (uint8_t*)&scale.tare_offset, sizeof(int32_t));
    printf("Calibration loaded: factor=%.2f, tare=%ld\r\n",
           scale.calibration_factor, scale.tare_offset);
}
```

---

## การใช้งานขั้นสูง

### อ่านค่าเฉลี่ยเพื่อลด noise

```c
float weight;
HX711_GetWeightAvg(&scale, 10, &weight);  // เฉลี่ย 10 ครั้ง
printf("Average: %.2f g\r\n", weight);
```

### เปลี่ยน Gain

```c
// Gain 128 (default) — CH-A, sensitivity สูงสุด
HX711_SetGain(&scale, HX711_GAIN_128);

// Gain 64 — CH-A, sensitivity ครึ่งหนึ่ง
HX711_SetGain(&scale, HX711_GAIN_64);

// Gain 32 — CH-B, ใช้สำหรับ load cell ตัวที่ 2
HX711_SetGain(&scale, HX711_GAIN_32);
```

### Power Save Mode

```c
// ประหยัดไฟเมื่อไม่ใช้
HX711_PowerDown(&scale);
printf("HX711 powered down\r\n");

// รอ...
Delay_Ms(5000);

// เปิดกลับมา
HX711_PowerUp(&scale);
HX711_Tare(&scale, 5);  // tare ใหม่หลัง power up
```

### Load Cell พร้อม auto-tare ทุก N วินาที

```c
uint32_t last_tare = 0;

while (1) {
    // auto tare ทุก 30 วินาที (กรณีมี drift)
    if (Get_CurrentMs() - last_tare > 30000) {
        // ตรวจสอบว่าไม่มีน้ำหนักบน scale (น้ำหนัก < 5g)
        float w;
        HX711_GetWeight(&scale, &w);
        if (w > -5.0f && w < 5.0f) {
            HX711_Tare(&scale, 5);
            last_tare = Get_CurrentMs();
        }
    }

    float weight;
    HX711_GetWeightAvg(&scale, 5, &weight);
    printf("%.1f g\r\n", weight);
    Delay_Ms(200);
}
```

---

## Troubleshooting

### ปัญหา: ค่า weight เป็น 0 หรือ ค่า raw ไม่เปลี่ยน

| สาเหตุ | วิธีแก้ |
|--------|---------|
| สาย load cell ต่อผิด | ตรวจสอบสี E+/E-/A+/A- |
| DOUT/SCK pin ผิด | ตรวจ `HX711_Init()` ว่า pin ถูก |
| HX711 ไม่มีไฟ | ตรวจ VCC=3.3V และ GND |

### ปัญหา: HX711_ERROR_TIMEOUT

| สาเหตุ | วิธีแก้ |
|--------|---------|
| SCK ค้างอยู่ HIGH | ตรวจ pin SCK ว่าเป็น OUTPUT และ LOW ก่อน Init |
| RATE pin ต่อ HIGH (80Hz) | ที่ 80Hz HX711 อาจ timeout บางครั้งถ้า MCU ช้า |
| HX711 เสียหาย | ลอง chip ใหม่ |

### ปัญหา: ค่าน้ำหนักไม่นิ่ง (fluctuate มาก)

| สาเหตุ | วิธีแก้ |
|--------|---------|
| Noise จากไฟฟ้า | ต่อ capacitor 100nF ที่ E+ และ A+ ไป GND |
| Interrupt กวน timing | Library ใช้ `__disable_irq()` แล้ว — ตรวจว่า IRQ ไม่ยาวเกิน |
| ใช้ samples น้อยเกินไป | ใช้ `HX711_GetWeightAvg(&s, 10, &w)` |
| Load cell ราคาถูก | ลอง load cell quality ดีกว่า หรือเพิ่ม filter software |

### ปัญหา: ค่า calibration ผิด (น้ำหนักไม่ตรง)

| สาเหตุ | วิธีแก้ |
|--------|---------|
| Tare ยังไม่ได้ทำก่อน calibration | ทำ Tare ก่อนเสมอ |
| ใช้น้ำหนัก reference ผิด | ตรวจสอบน้ำหนัก reference ด้วยตาชั่งที่เชื่อถือได้ |
| calibration factor ผิดเครื่องหมาย | ถ้าค่าติดลบ ลอง factor = -factor |

---

## API Reference

### `HX711_Init(hx, pin_dout, pin_sck)` → `HX711_Status`
เริ่มต้น HX711 — ตั้ง pin mode และอ่าน 1 ครั้งเพื่อ apply gain

---

### `HX711_IsReady(hx)` → `uint8_t`
ตรวจสอบว่าพร้อมอ่าน (DOUT=LOW) → 1=พร้อม, 0=ยังไม่พร้อม

---

### `HX711_Read(hx, &result)` → `HX711_Status`
อ่านค่า raw 24-bit (blocking, รอจน ready)

---

### `HX711_Tare(hx, samples)` → `HX711_Status`
ตั้งศูนย์ด้วยการเฉลี่ย N samples (`samples`: 1-20)

---

### `HX711_SetCalibration(hx, factor)`
ตั้ง calibration factor (raw units per gram)

---

### `HX711_GetWeight(hx, &weight)` → `HX711_Status`
อ่านน้ำหนัก (gram) = `(raw - tare) / factor`

---

### `HX711_GetWeightAvg(hx, samples, &weight)` → `HX711_Status`
อ่านน้ำหนักเฉลี่ย N samples

---

### `HX711_SetGain(hx, gain)` → `HX711_Status`
เปลี่ยน Gain — มีผลในการอ่านครั้งถัดไป

| Gain | ค่า | ช่อง | หมายเหตุ |
|------|-----|------|----------|
| `HX711_GAIN_128` | 128 | CH-A | default, sensitivity สูงสุด |
| `HX711_GAIN_64` | 64 | CH-A | |
| `HX711_GAIN_32` | 32 | CH-B | สำหรับ load cell ตัวที่ 2 |

---

### `HX711_PowerDown(hx)` → `HX711_Status`
เข้า power down (SCK=HIGH > 60µs)

---

### `HX711_PowerUp(hx)` → `HX711_Status`
ออกจาก power down (SCK=LOW, รอ 500ms)

---

### HX711_Status Values

| ค่า | ความหมาย |
|-----|----------|
| `HX711_OK` | สำเร็จ |
| `HX711_ERROR_PARAM` | Parameter ผิด (NULL pointer หรือไม่ได้ Init) |
| `HX711_ERROR_TIMEOUT` | รอ DOUT=LOW นานเกิน 1000ms |

---

> **หมายเหตุ:** ไลบรารีนี้ถูกพอร์ตจากโปรเจกต์ CH32V003 ผ่าน Compatibility Shim  
> CH57x (CH570D) มีเฉพาะ PORTA (PA0–PA15) — ต้องปรับ pin mapping จาก PCx/PDx เป็น PAx  
> ดูรายละเอียดได้ที่ [compat_shim.md](../../SimpleHAL/docs/compat_shim.md)