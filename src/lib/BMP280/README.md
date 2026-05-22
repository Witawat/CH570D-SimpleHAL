# BMP280 Temperature & Pressure Sensor Library

> **Library สำหรับใช้งาน BMP280 บน CH57x**

## ภาพรวม

BMP280 เป็น sensor วัดอุณหภูมิและความดันอากาศความแม่นยำสูงจาก Bosch Sensortec  
สามารถใช้คำนวณ **ความสูง (Altitude)** โดยประมาณจากความดันอากาศได้

| คุณสมบัติ | ค่า |
|----------|-----|
| I2C Address | 0x76 (SDO=GND) หรือ 0x77 (SDO=VCC) |
| อุณหภูมิ | -40 ถึง +85°C, ±1°C, 0.01°C resolution |
| ความดัน | 300–1100 hPa, ±1 hPa |
| ไฟเลี้ยง | 1.71–3.6V |

---

## Hardware Setup

```
CH57x          BMP280 Module
PC2 (SCL) ------> SCK  [4.7kΩ → 3.3V]
PC1 (SDA) <-----> SDA  [4.7kΩ → 3.3V]
3.3V -----------> VCC (3.3V)
GND ------------> GND
GND ------------> SDO  → address 0x76
```

> ⚠️ BMP280 ทำงานที่ **3.3V สูงสุด** — ห้ามต่อ 5V

---

## การใช้งานพื้นฐาน

```c
#include "SimpleHAL.h"
#include "BMP280.h"

BMP280_Instance bmp;

int main(void) {
    SystemCoreClockUpdate();
    Timer_Init();
    I2C_SimpleInit(I2C_100KHZ, I2C_PINS_DEFAULT);

    if (BMP280_Init(&bmp, BMP280_ADDR_LOW) != BMP280_OK) {
        printf("BMP280 not found!\r\n");
        while (1) {}
    }

    while (1) {
        float temp, press;
        BMP280_Read(&bmp, &temp, &press);
        float alt = BMP280_GetAltitude(&bmp, press, 1013.25f);

        printf("T=%.2f C  P=%.2f hPa  Alt=%.1f m\r\n", temp, press, alt);
        Delay_Ms(1000);
    }
}
```

---

## การใช้งานขั้นสูง

### Forced Mode (อ่านครั้งเดียว → sleep)

```c
// ประหยัดไฟ: ตื่นขึ้น, อ่าน, กลับ sleep
BMP280_SetMode(&bmp, BMP280_MODE_FORCED, BMP280_OS_X1, BMP280_OS_X1);
Delay_Ms(10);  // รอ measurement เสร็จ
float temp, press;
BMP280_Read(&bmp, &temp, &press);
// BMP280 กลับ sleep อัตโนมัติ
```

### ปรับ Oversampling เพื่อความแม่นยำ

```c
// ความแม่นยำสูง (ช้ากว่า)
BMP280_SetMode(&bmp, BMP280_MODE_NORMAL, BMP280_OS_X16, BMP280_OS_X16);

// Fast mode (noise มากกว่า)
BMP280_SetMode(&bmp, BMP280_MODE_NORMAL, BMP280_OS_X1, BMP280_OS_X1);
```

### Weather Station Preset

```c
// ตั้งค่าสำหรับ weather monitoring (แนะนำจาก Bosch)
BMP280_SetMode(&bmp, BMP280_MODE_NORMAL, BMP280_OS_X1, BMP280_OS_X1);
BMP280_SetFilter(&bmp, BMP280_FILTER_OFF);
// Standby 1000ms (ตั้งใน Init ได้)
```

---

## Troubleshooting

| ปัญหา | สาเหตุ | วิธีแก้ |
|-------|--------|---------|
| `BMP280_ERROR_CHIPID` | ชิปไม่ใช่ BMP280 | ตรวจ VCC (ต้อง 3.3V), ลอง address 0x77 |
| `BMP280_ERROR_I2C` | I2C ไม่เชื่อม | ตรวจ pull-up, ตรวจ SDO pin |
| ค่าความดันผิดมาก | t_fine ยังเป็น 0 | ต้องเรียก `BMP280_Read()` อย่างน้อย 1 ครั้งก่อน |
| ความสูงผิด | sea_level_pa ไม่ถูก | ตรวจสอบความดันจริงที่ระดับน้ำทะเลในวันนั้น |

---

## API Reference

| Function | คำอธิบาย |
|----------|----------|
| `BMP280_Init(bmp, addr)` | Init — ตรวจ CHIP_ID, โหลด calibration |
| `BMP280_Read(bmp, &temp, &press)` | อ่านอุณหภูมิ (°C) และความดัน (hPa) |
| `BMP280_GetAltitude(bmp, press, sea_level)` | คำนวณความสูง (m) |
| `BMP280_SetMode(bmp, mode, os_temp, os_press)` | ตั้ง mode และ oversampling |
| `BMP280_SetFilter(bmp, filter)` | ตั้ง IIR filter coefficient |
| `BMP280_Reset(bmp)` | Soft reset + reload calibration |

---

> **หมายเหตุ:** ไลบรารีนี้ถูกพอร์ตจากโปรเจกต์ CH32V003 ผ่าน Compatibility Shim  
> CH57x (CH570D) มีเฉพาะ PORTA (PA0–PA15) — ต้องปรับ pin mapping จาก PCx/PDx เป็น PAx  
> ดูรายละเอียดได้ที่ [compat_shim.md](../../SimpleHAL/docs/compat_shim.md)