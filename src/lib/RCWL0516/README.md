# RCWL-0516 Microwave Radar Sensor สำหรับ CH57x series

## ภาพรวม

RCWL-0516 ใช้ Doppler microwave radar ความถี่ ~3.18 GHz ตรวจจับการเคลื่อนไหว
ต่างจาก PIR ตรงที่ตรวจได้ผ่านผนัง/พลาสติกบางๆ และไม่ต้องการ warm-up time

## 1) วงจรต่อใช้งาน

### พื้นฐาน
```
CH57x/006              RCWL-0516
─────────────────────────────────────────────────────
PD2 (input) ←─────────── OUT  (3.3V logic out)
5V ──────────────────────VIN  (ต้องการ 4V-28V)
GND ─────────────────────GND
(ห้ามต่อ 3V3 ของ MCU → VIN เด็ดขาด)

⚠️ 3V3 บน module เป็น OUTPUT ของ module ไม่ใช่ VCC input
⚠️ OUT ≈ 3.3V เมื่อ VIN=5V → ปลอดภัยกับ GPIO 3.3V ของ CH57x
```

### พร้อม CDS (ทำงานเฉพาะกลางคืน)
```
3V3 (module) ──[LDR 10kΩ]── CDS ──[10kΩ]── GND

กลางวัน : LDR ต้านทานต่ำ → CDS ≈ HIGH → radar หยุดทำงาน
กลางคืน : LDR ต้านทานสูง → CDS ≈ LOW  → radar ทำงานปกติ
```

## 2) ความแตกต่างจาก PIR ธรรมดา

| คุณสมบัติ | RCWL-0516 | PIR (NS312) |
|---|---|---|
| เซ็นเซอร์ | Doppler radar 3.18GHz | Pyroelectric |
| ตรวจผ่านวัสดุ | ได้ (พลาสติก, ไม้บาง) | ไม่ได้ |
| Warm-up | ไม่ต้องการ | ~30-60 วินาที |
| Hold time default | ~2 วินาที (RC hardware) | ~5-10 วินาที |
| รัศมี (ทิศทาง) | 360° horizontal | ~110°-140° cone |
| การรบกวนจาก LED | น้อยมาก | ต้องระวัง |
| ไฟเลี้ยง | 4V-28V (ต้องการ 4V+) | 3.3V-5V |
| False trigger | สูงกว่าในที่รบกวน RF | ต่ำกว่า |

## 3) ตัวอย่างโค้ด

### พื้นฐาน (polling)
```c
#include "User/Lib/RCWL0516/RCWL0516.h"

RCWL0516_Instance radar;

int main(void) {
    SystemCoreClockUpdate();
    Timer_Init();

    RCWL0516_Init(&radar, PD2);

    while (1) {
        RCWL0516_Update(&radar);

        if (RCWL0516_IsMotionDetected(&radar)) {
            // มีการเคลื่อนไหว (รวม software hold)
        }
    }
}
```

### พร้อม callback
```c
RCWL0516_Instance radar;

void on_motion_start(void) {
    // ทำงานเมื่อตรวจพบครั้งแรก
}

void on_motion_end(void) {
    uint32_t dur = RCWL0516_GetLastDurationMs(&radar);
    // dur คือระยะเวลาที่ active ล่าสุด
}

int main(void) {
    SystemCoreClockUpdate();
    Timer_Init();

    RCWL0516_Init(&radar, PD2);
    RCWL0516_SetCallbacks(&radar, on_motion_start, NULL, on_motion_end);
    RCWL0516_SetHoldTime(&radar, 5000); // software hold 5 วินาที

    while (1) {
        RCWL0516_Update(&radar);
    }
}
```

### Config ละเอียด
```c
RCWL0516_Config cfg = {
    .out_pin      = PD2,
    .cds_pin      = RCWL0516_PIN_NONE,  // ไม่ใช้ CDS
    .hold_ms      = 3000,               // software hold 3 วินาที
    .debounce_ms  = 80,                 // debounce 80ms
    .filter_count = 5                   // majority vote 5 samples
};

RCWL0516_Instance radar;
RCWL0516_InitWithConfig(&radar, &cfg);
```

### นับสถิติ
```c
RCWL0516_Update(&radar);

uint32_t n   = RCWL0516_GetTotalTriggers(&radar);
uint32_t dur = RCWL0516_GetLastDurationMs(&radar);
uint32_t ago = RCWL0516_GetMsSinceLastTrigger(&radar);
```

## 4) API Reference

| Function | พารามิเตอร์ | คืนค่า | รายละเอียด |
|---|---|---|---|
| `RCWL0516_Init` | instance, pin | status | init ด้วยค่า default |
| `RCWL0516_InitWithConfig` | instance, config* | status | init พร้อม config |
| `RCWL0516_SetCallbacks` | instance, on_triggered, on_active, on_idle | status | ตั้ง callbacks |
| `RCWL0516_Update` | instance | void | ต้องเรียกทุก loop |
| `RCWL0516_IsMotionDetected` | instance | 0/1 | ตรวจสอบ motion |
| `RCWL0516_ReadRaw` | instance | 0/1 | อ่าน GPIO ตรง |
| `RCWL0516_GetState` | instance | state enum | อ่าน state machine |
| `RCWL0516_GetTotalTriggers` | instance | uint32 | นับ trigger ทั้งหมด |
| `RCWL0516_GetLastDurationMs` | instance | uint32 | ระยะเวลา active ล่าสุด |
| `RCWL0516_GetMsSinceLastTrigger` | instance | uint32 | ms นับแต่ trigger ล่าสุด |
| `RCWL0516_SetHoldTime` | instance, ms | status | ปรับ software hold |
| `RCWL0516_Reset` | instance | status | reset state กลับ IDLE |

## 5) State Machine

```
                ┌──────┐
    signal HIGH │      │ 1 round
                ▼      │
  IDLE ──────► TRIGGERED ──────► ACTIVE
   ▲                                │
   │ hold หมด                       │ signal ลง
   │                                ▼
   └────────────────────── HOLDING ◄──
                                 │ retrigger (signal HIGH)
                                 └──────────► ACTIVE
```

- **IDLE**: ไม่มีการเคลื่อนไหว
- **TRIGGERED**: rising edge ใหม่ (1 rount transition)
- **ACTIVE**: signal HIGH อยู่ (retrigger ได้เรื่อยๆ)
- **HOLDING**: signal ลงแล้วแต่ยัง hold ด้วย software

## 6) Hardware Tuning บน PCB

| Component | ตำแหน่ง | Effect | วิธีปรับ |
|---|---|---|---|
| C-TM (1nF default) | ใกล้ BISS0001 | ความไวในการตรวจจับ | เพิ่ม C → ไวขึ้น |
| R-GN (1MΩ) | pad GN | Hardware hold time | เพิ่ม R → hold นานขึ้น |
| C-CDS | ใกล้ CDS pin | RC กรอง light signal | — |
| CDIV jumper | pad CDIV | หาร output freq ÷2 | บัดกรี pad → enable |

## 7) Troubleshooting

| อาการ | สาเหตุ | วิธีแก้ |
|---|---|---|
| ไม่ตอบสนองเลย | VIN < 4V | ใช้ 5V เลี้ยง module |
| Trigger ตลอดเวลา | RF interference, พัดลม, AC ใกล้ๆ | ย้าย module ห่างจากแหล่งรบกวน, ลด sensitivity |
| Hold นานเกินไป | R-GN + C2 บน PCB | เพิ่ม `hold_ms=0` ใน software หรือลด RC hardware |
| ตรวจไม่ครบ 360° | ด้านหน้า module ไวกว่า | หันหน้า PCB (IC side) ไปทิศที่ต้องการ |
| False trigger ตอนเปิด | module เริ่มต้น | รอ 1-2 วินาทีก่อนเริ่ม logic หรือใช้ `RCWL0516_Reset()` |
| CDS ไม่ทำงาน | ต่อวงจรผิด | ตรวจ LDR divider, CDS ต้องต่ำ (≤0.5V) เพื่อ enable |

---

> **หมายเหตุ:** ไลบรารีนี้ถูกพอร์ตจากโปรเจกต์ CH32V003 ผ่าน Compatibility Shim  
> CH57x (CH570D) มีเฉพาะ PORTA (PA0–PA15) — ต้องปรับ pin mapping จาก PCx/PDx เป็น PAx  
> ดูรายละเอียดได้ที่ [compat_shim.md](../../SimpleHAL/docs/compat_shim.md)