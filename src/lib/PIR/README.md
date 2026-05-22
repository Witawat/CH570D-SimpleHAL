# PIR Motion Sensor Library

> **Library สำหรับจัดการเซนเซอร์ตรวจจับความเคลื่อนไหว (PIR) เช่น HC-SR501, NS312 และเซนเซอร์เรดาร์ RCWL-0516 สำหรับ CH57x**

## 📋 สารบัญ

1. [ภาพรวม](#ภาพรวม)
2. [Hardware Setup](#hardware-setup)
3. [การใช้งานพื้นฐาน](#การใช้งานพื้นฐาน)
4. [เทคนิคขั้นสูง (Debounce & Callbacks)](#เทคนิคขั้นสูง-debounce--callbacks)
5. [API Reference](#api-reference)

---

## ภาพรวม

PIR (Passive Infrared) และ Microwave Radar เป็นเซนเซอร์ที่ให้สัญญาณ Digital (High/Low) เมื่อตรวจพบการเคลื่อนไหว อย่างไรก็ตาม สัญญาณเหล่านี้มักจะมี "ความไม่นิ่ง" (Noise) หรือ "การกระเด้ง" (Bouncing)  
Library นี้จึงออกแบบมาเพื่อแก้ปัญหาดังกล่าวด้วยระบบ **Debounce**, **Filtering** และ **State Machine** เพื่อให้การตรวจจับแม่นยำที่สุด

---

## Hardware Setup

### การเชื่อมต่อ

| PIR Sensor Pin | CH57x Pin | หมายเหตุ |
|----------------|--------------|----------|
| VCC            | 3.3V / 5V    | แรงดันไฟเลี้ยง (ตามสเปกของโมดูล) |
| GND            | GND          | กราวด์ร่วม |
| OUT / SIGNAL   | Any GPIO     | สัญญาณดิจิทัลขาออก (เช่น PC4) |

---

## การใช้งานพื้นฐาน

```c
#include "main.h"
#include "Lib/PIR/PIR.h"

PIR_Instance* pir;

int main(void) {
    SystemCoreClockUpdate();
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1);
    Timer_Init();

    // 1. เริ่มต้น PIR บนขา PC4
    pir = PIR_Init(PC4);

    while(1) {
        // 2. อัปเดตสถานะเซนเซอร์ (ต้องเรียกสม่ำเสมอใน loop)
        PIR_Update();

        // 3. ตรวจสอบสถานะ
        if (PIR_IsMotionDetected(pir)) {
            // ทำงานเมื่อตรวจพบการเคลื่อนไหว
            printf("Motion Detected!\r\n");
        }
    }
}
```

---

## เทคนิคขั้นสูง (Debounce & Callbacks)

เพื่อป้องกันการทำงานผิดพลาดจากสัญญาณรบกวน (เช่น ไฟกระชากเมื่อเปิด Relay) Library นี้รองรับการตั้งค่าที่ละเอียดขึ้น:

```c
void on_start(void) { printf("Motion Started\r\n"); }
void on_end(void) { printf("Motion Ended\r\n"); }

int main(void) {
    pir = PIR_Init(PC4);
    
    // ตั้งค่าเวลา Debounce (ป้องกันสัญญาณกระเพื่อมสั้นๆ)
    PIR_SetDebounce(pir, 150); // 150ms
    
    // ตั้งค่า Callback เมื่อเกิดเหตุการณ์
    PIR_SetCallbacks(pir, on_start, on_end, NULL);
    
    while(1) {
        PIR_Update();
    }
}
```

---

## API Reference

- `PIR_Init(pin)` : เริ่มต้นเซนเซอร์บนขาที่กำหนด
- `PIR_Update()` : ประมวลผลสถานะเซนเซอร์ (ต้องเรียกใน `while(1)`)
- `PIR_IsMotionDetected(pir)` : คืนค่า `true` เมื่อมีการเคลื่อนไหว (ผ่านการกรองแล้ว)
- `PIR_SetDebounce(pir, ms)` : ตั้งเวลาหน่วงเพื่อยืนยันสถานะ
- `PIR_SetTimeout(pir, ms)` : ตั้งเวลาที่จะถือว่าการเคลื่อนไหวสิ้นสุดลง
- `PIR_SetCallbacks(pir, start, end, active)` : ตั้งฟังก์ชันให้ทำงานอัตโนมัติตามสถานะ
- `PIR_SetFilterLevel(pir, level)` : ตั้งระดับการกรองสัญญาณรบกวน (None, Low, Medium, High)

---
**พัฒนาโดย:** CH57x Library Team
**รองรับบอร์ด:** CH57x Development Board

---

> **หมายเหตุ:** ไลบรารีนี้ถูกพอร์ตจากโปรเจกต์ CH32V003 ผ่าน Compatibility Shim  
> CH57x (CH570D) มีเฉพาะ PORTA (PA0–PA15) — ต้องปรับ pin mapping จาก PCx/PDx เป็น PAx  
> ดูรายละเอียดได้ที่ [compat_shim.md](../../SimpleHAL/docs/compat_shim.md)