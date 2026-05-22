# DHT11 / DHT22 Temperature & Humidity Sensor Library

> **Library สำหรับอ่านค่าอุณหภูมิและความชื้นจาก DHT11 และ DHT22 บน CH57x**

## 📋 สารบัญ

1. [ภาพรวม](#ภาพรวม)
2. [คุณสมบัติ](#คุณสมบัติ)
3. [เปรียบเทียบ DHT11 vs DHT22](#เปรียบเทียบ-dht11-vs-dht22)
4. [Hardware Setup](#hardware-setup)
5. [DHT Protocol](#dht-protocol)
6. [การใช้งานพื้นฐาน](#การใช้งานพื้นฐาน)
7. [การใช้งานขั้นสูง](#การใช้งานขั้นสูง)
8. [Troubleshooting](#troubleshooting)
9. [API Reference](#api-reference)

---

## ภาพรวม

DHT11 และ DHT22 เป็น sensor วัดอุณหภูมิและความชื้นแบบ digital ที่นิยมใช้กันมาก ใช้สายสัญญาณเพียงเส้นเดียว (single-wire protocol) สำหรับการส่งข้อมูล

### ความแตกต่างหลัก

| คุณสมบัติ | DHT11 | DHT22 / AM2302 |
|-----------|-------|----------------|
| ราคา | ถูกกว่า | แพงกว่าเล็กน้อย |
| ช่วงอุณหภูมิ | 0-50°C | -40 ถึง 80°C |
| ความแม่นยำอุณหภูมิ | ±2°C | ±0.5°C |
| ความละเอียดอุณหภูมิ | 1°C | 0.1°C |
| ช่วงความชื้น | 20-90%RH | 0-100%RH |
| ความแม่นยำความชื้น | ±5% | ±2-5% |
| ความถี่อ่านสูงสุด | 1 ครั้ง/วินาที | 0.5 ครั้ง/วินาที |
| อุณหภูมิติดลบ | ไม่รองรับ | รองรับ |

---

## คุณสมบัติ

- ✅ รองรับ DHT11 และ DHT22 / AM2302 ในไฟล์เดียว
- ✅ อ่านอุณหภูมิและความชื้นพร้อมกันในครั้งเดียว
- ✅ CRC checksum validation ป้องกันข้อมูลเสียหาย
- ✅ Status reporting ละเอียด (Timeout, Checksum, Not Ready)
- ✅ รองรับสูงสุด 4 sensors พร้อมกัน
- ✅ Non-blocking ready check ด้วย `DHT_IsReady()`
- ✅ เอกสารภาษาไทยครบถ้วน

---

## Hardware Setup

### การต่อวงจร

```
DHT11 / DHT22 Pinout:
  +-------+
  |       |
  | 1 VCC |-----> 3.3V
  | 2 DAT |-----> GPIO Pin (เช่น PC4)
  | 3 N/C |      (ไม่ต้องต่อ)
  | 4 GND |-----> GND
  +-------+

Pull-up Resistor:
  3.3V
   |
  [10kΩ]  <--- จำเป็นมาก! ห้ามข้าม
   |
   +------> GPIO Pin (DATA)
   |
  DHT DATA Pin
```

### ข้อกำหนดสำคัญ

> ⚠️ **ต้องต่อ pull-up resistor 10kΩ เสมอ** ระหว่าง VCC และ DATA pin  
> หากไม่ต่อ pull-up จะเกิด Timeout error

- ใช้ 3.3V (ไม่ใช่ 5V) เพราะ CH57x ทำงานที่ 3.3V
- สายสัญญาณสั้นกว่า 20 เมตร (ยาวเกินใช้ 4.7kΩ แทน 10kΩ)
- ห้ามต่อ DHT22 หลายตัวบน pin เดียว (ใช้คนละ pin)

### GPIO Pins ที่ใช้ได้

สามารถใช้ GPIO pin ใดก็ได้บน CH57x:
- `PA1`, `PA2`
- `PC0` ถึง `PC7`
- `PD2` ถึง `PD7`

---

## DHT Protocol

### หลักการทำงาน

DHT ใช้ single-wire protocol โดย Host (CH57x) เป็นผู้เริ่มการสื่อสาร:

```
1. HOST ดึง DATA ลง LOW เป็นเวลา 18-20ms (Start Signal)
   ___     _______________________
      |___|
      18ms+

2. HOST ปล่อย bus (pull-up ดึงขึ้น HIGH), รอ 40µs
   _________________________
                            |
                           40µs

3. DHT ตอบสนอง: LOW 80µs → HIGH 80µs
   _______     _______     _______
          |___|       |___|
           80µs        80µs

4. DHT ส่งข้อมูล 40 bits:
   - แต่ละ bit เริ่มด้วย LOW ~50µs
   - Bit '0': HIGH 26-28µs
   - Bit '1': HIGH 70µs

   ___    ____     ___    __________
      |__|    |   |   |__|          |
      50µs 26µs   50µs    70µs
        (bit=0)             (bit=1)
```

### โครงสร้างข้อมูล 40 bits

```
[Byte 0: Humidity Integer] [Byte 1: Humidity Decimal]
[Byte 2: Temp Integer]     [Byte 3: Temp Decimal]
[Byte 4: Checksum = Byte0 + Byte1 + Byte2 + Byte3]

DHT11: Byte 1 และ Byte 3 มักเป็น 0 เสมอ (ไม่มีทศนิยม)
DHT22: Humidity = (Byte0 << 8 | Byte1) × 0.1
       Temp     = (Byte2 << 8 | Byte3) × 0.1  (Bit15=1 คือลบ)
```

---

## การใช้งานพื้นฐาน

### ขั้นตอนที่ 1: Include และประกาศตัวแปร

```c
#include "SimpleHAL.h"
#include "DHT.h"

DHT_Instance dht;  // ประกาศเป็น global หรือ static
```

### ขั้นตอนที่ 2: Init ใน main()

```c
int main(void) {
    SystemCoreClockUpdate();
    Timer_Init();  // จำเป็นสำหรับ Delay และ Get_CurrentMs()

    // Init DHT22 บน pin PC4
    DHT_Init(&dht, PC4, DHT_TYPE_DHT22);

    // รอ 2 วินาทีให้ sensor stabilize หลัง power-up
    Delay_Ms(2000);

    while (1) {
        // อ่านค่า (ใช้เวลาประมาณ 20-25ms)
        DHT_Status status = DHT_Read(&dht);

        if (status == DHT_OK) {
            float temp = DHT_GetTemperature(&dht);
            float hum  = DHT_GetHumidity(&dht);
            printf("อุณหภูมิ: %.1f C  ความชื้น: %.1f%%\r\n", temp, hum);
        } else {
            printf("Error: %s\r\n", DHT_StatusStr(status));
        }

        Delay_Ms(2000);  // รอ 2 วินาทีก่อนอ่านครั้งต่อไป
    }
}
```

### ตัวอย่าง DHT11

```c
DHT_Instance dht11;
DHT_Init(&dht11, PC3, DHT_TYPE_DHT11);

DHT_Read(&dht11);
printf("Temp: %.0f C  Hum: %.0f%%\r\n",
       DHT_GetTemperature(&dht11),
       DHT_GetHumidity(&dht11));
```

---

## การใช้งานขั้นสูง

### ใช้หลาย Sensor พร้อมกัน (สูงสุด 4 ตัว)

```c
DHT_Instance dht_indoor;   // sensor ในบ้าน
DHT_Instance dht_outdoor;  // sensor นอกบ้าน

DHT_Init(&dht_indoor,  PC3, DHT_TYPE_DHT22);
DHT_Init(&dht_outdoor, PC4, DHT_TYPE_DHT22);

// อ่านทีละตัว (ห้ามอ่านพร้อมกัน)
DHT_Read(&dht_indoor);
Delay_Ms(100);  // เว้นระยะเล็กน้อย
DHT_Read(&dht_outdoor);

printf("ใน: %.1f C  นอก: %.1f C\r\n",
       DHT_GetTemperature(&dht_indoor),
       DHT_GetTemperature(&dht_outdoor));
```

### ใช้ DHT_IsReady() แบบ Non-Blocking

```c
Timer_t display_timer;
Start_Timer(&display_timer, 500, 1);  // แสดงผลทุก 500ms

while (1) {
    // อ่านค่าเมื่อพร้อม (ไม่บล็อก loop)
    if (DHT_IsReady(&dht)) {
        DHT_Read(&dht);
    }

    // แสดงผลทุก 500ms
    if (Is_Timer_Expired(&display_timer)) {
        printf("T=%.1fC H=%.1f%%\r\n",
               DHT_GetTemperature(&dht),
               DHT_GetHumidity(&dht));
    }

    // ทำงานอื่นๆ ได้เลยไม่ถูก block
}
```

### แจ้งเตือนเมื่ออุณหภูมิเกินกำหนด

```c
#define TEMP_HIGH_ALARM  35.0f  // แจ้งเตือนเมื่อ > 35°C
#define HUM_HIGH_ALARM   80.0f  // แจ้งเตือนเมื่อ > 80%

void check_environment(void) {
    if (DHT_Read(&dht) != DHT_OK) return;

    float t = DHT_GetTemperature(&dht);
    float h = DHT_GetHumidity(&dht);

    if (t > TEMP_HIGH_ALARM) {
        printf("⚠️ อุณหภูมิสูง! %.1f C\r\n", t);
        // เปิดพัดลม, ส่ง alert ฯลฯ
    }

    if (h > HUM_HIGH_ALARM) {
        printf("⚠️ ความชื้นสูง! %.1f%%\r\n", h);
    }
}
```

### Error Handling แบบครบถ้วน

```c
uint8_t retry_count = 0;

while (retry_count < 3) {
    DHT_Status st = DHT_Read(&dht);

    if (st == DHT_OK) {
        printf("T=%.1f H=%.1f\r\n",
               DHT_GetTemperature(&dht),
               DHT_GetHumidity(&dht));
        retry_count = 0;
        break;

    } else if (st == DHT_ERROR_NOT_READY) {
        Delay_Ms(500);  // รอแล้วลองใหม่

    } else if (st == DHT_ERROR_TIMEOUT) {
        printf("Timeout! ตรวจสอบการต่อ pull-up 10kΩ\r\n");
        retry_count++;
        Delay_Ms(100);

    } else if (st == DHT_ERROR_CHECKSUM) {
        printf("Checksum error! ตรวจสอบสายสัญญาณ\r\n");
        retry_count++;
        Delay_Ms(100);
    }
}
```

---

## Troubleshooting

### ปัญหา: ได้ `TIMEOUT` error ตลอด

| สาเหตุ | วิธีแก้ |
|--------|---------|
| ไม่มี pull-up resistor | ต่อ 10kΩ ระหว่าง VCC และ DATA |
| ไฟเลี้ยงผิด | ใช้ 3.3V ไม่ใช่ 5V |
| Pin ผิด | ตรวจสอบ pin ใน DHT_Init() |
| Sensor เสีย | ทดสอบด้วย sensor ตัวอื่น |

### ปัญหา: ได้ `CHECKSUM_ERROR` บางครั้ง

| สาเหตุ | วิธีแก้ |
|--------|---------|
| สายยาวเกินไป | ลด pull-up เป็น 4.7kΩ |
| Noise มาก | ใช้สายสั้นลง หรือเพิ่ม capacitor 100nF |
| Interrupt รบกวน | Library จัดการให้แล้ว |

### ปัญหา: ได้ `NOT_READY` error

ปกติมาก! เกิดเมื่ออ่านค่าถี่เกินไป ต้องรออย่างน้อย 2 วินาที  
แก้ด้วยการใช้ `DHT_IsReady()` ก่อนเรียก `DHT_Read()`

---

## API Reference

### `DHT_Init(dht, pin, type)`
เริ่มต้น DHT sensor

| Parameter | Type | คำอธิบาย |
|-----------|------|----------|
| `dht` | `DHT_Instance*` | ตัวแปร instance |
| `pin` | `uint8_t` | GPIO pin (เช่น `PC4`, `PD3`) |
| `type` | `DHT_Type` | `DHT_TYPE_DHT11` หรือ `DHT_TYPE_DHT22` |

---

### `DHT_Read(dht)` → `DHT_Status`
อ่านค่าอุณหภูมิและความชื้น ใช้เวลาประมาณ 20-25ms

---

### `DHT_GetTemperature(dht)` → `float`
ดึงค่าอุณหภูมิล่าสุด (°C) — ต้องเรียก `DHT_Read()` ก่อน

---

### `DHT_GetHumidity(dht)` → `float`
ดึงค่าความชื้นล่าสุด (%RH) — ต้องเรียก `DHT_Read()` ก่อน

---

### `DHT_IsReady(dht)` → `bool`
ตรวจสอบว่าผ่านมา 2 วินาทีหรือยัง (non-blocking)

---

### `DHT_StatusStr(status)` → `const char*`
แปลง status code เป็นข้อความ เช่น `"OK"`, `"TIMEOUT"`

---

### DHT_Status Values

| ค่า | ความหมาย |
|-----|----------|
| `DHT_OK` | อ่านสำเร็จ |
| `DHT_ERROR_TIMEOUT` | Sensor ไม่ตอบสนอง ตรวจสอบการต่อ |
| `DHT_ERROR_CHECKSUM` | ข้อมูลเสียหาย ตรวจสอบสาย |
| `DHT_ERROR_NOT_READY` | รออีก 2 วินาที |
| `DHT_ERROR_NOT_INIT` | ยังไม่ได้เรียก `DHT_Init()` |

---

> **หมายเหตุ:** ไลบรารีนี้ถูกพอร์ตจากโปรเจกต์ CH32V003 ผ่าน Compatibility Shim  
> CH57x (CH570D) มีเฉพาะ PORTA (PA0–PA15) — ต้องปรับ pin mapping จาก PCx/PDx เป็น PAx  
> ดูรายละเอียดได้ที่ [compat_shim.md](../../SimpleHAL/docs/compat_shim.md)