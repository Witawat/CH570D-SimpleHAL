# Push Button Library

> **Library สำหรับจัดการปุ่มกดแบบครบวงจร บน CH57x**

## 📋 สารบัญ

1. [ภาพรวม](#ภาพรวม)
2. [คุณสมบัติ](#คุณสมบัติ)
3. [Hardware Setup](#hardware-setup)
4. [หลักการทำงาน](#หลักการทำงาน)
5. [การใช้งานพื้นฐาน](#การใช้งานพื้นฐาน)
6. [การใช้งานขั้นสูง](#การใช้งานขั้นสูง)
7. [Polling Mode vs Callback Mode](#polling-mode-vs-callback-mode)
8. [Troubleshooting](#troubleshooting)
9. [API Reference](#api-reference)

---

## ภาพรวม

Push Button เป็น input device พื้นฐานที่สุด แต่การจัดการให้ถูกต้องต้องจัดการ:
- **Contact Bounce**: ขณะกด/ปล่อย pin จะสั่นหลายครั้งในช่วง 1-20ms
- **Long Press vs Short Press**: แยกการกดสั้นออกจากการกดค้าง
- **Double Click**: ตรวจจับการกดสองครั้งรวดเร็ว

Library นี้จัดการทั้งหมดให้อัตโนมัติ

---

## คุณสมบัติ

- ✅ Debounce อัตโนมัติ (default 20ms, ปรับได้)
- ✅ Single Press detection (กดครั้งเดียว)
- ✅ Release detection (ปล่อยปุ่ม)
- ✅ Long Press (กดค้าง, default 800ms, ปรับได้)
- ✅ Double Click (default window 300ms, ปรับได้)
- ✅ Multi Click (3, 4, ... ครั้งติดกัน)
- ✅ อ่านระยะเวลากดค้างแบบ milliseconds/seconds
- ✅ Boot Hold detection (กดค้างตั้งแต่เปิดระบบ)
- ✅ Callback system (6 events)
- ✅ Polling mode (ไม่ใช้ callback)
- ✅ รองรับ Active LOW และ Active HIGH
- ✅ Multi-button สูงสุด 8 ปุ่ม
- ✅ เอกสารภาษาไทยครบถ้วน

---

## Hardware Setup

### การต่อวงจร Active LOW (แนะนำ)

```
CH57x Pin (PC4)
    |
    |  ←—— Internal Pull-up (ถูก enable อัตโนมัติโดย library)
    |
    +——————[ Button ]——————> GND

สถานะ:
  ปุ่มปล่อย: PC4 = HIGH (pull-up ดึงขึ้น)
  ปุ่มกด:    PC4 = LOW  (ต่อลง GND)
```

### การต่อวงจร Active HIGH

```
VCC (3.3V)
    |
[ Button ]
    |
CH57x Pin ——[10kΩ pull-down]——> GND

สถานะ:
  ปุ่มปล่อย: Pin = LOW
  ปุ่มกด:    Pin = HIGH
```

### GPIO Pins ที่ใช้ได้

สามารถใช้ทุก GPIO pin:
- `PA1`, `PA2`
- `PC0` ถึง `PC7`  
- `PD2` ถึง `PD7`

---

## หลักการทำงาน

### Contact Bounce (ปัญหาที่ library แก้ให้)

```
สัญญาณจริงที่เกิดจากการกดปุ่ม (ก่อน debounce):

Pin:  ¯¯¯|_|¯|__|¯¯|_______________  ← bounce ขณะกด
           ↑
         ตรงนี้กด 1 ครั้ง แต่ pin เปลี่ยนหลายครั้ง!

หลัง debounce 20ms:

Pin:  ¯¯¯¯¯¯¯¯¯¯|_______________
                 ↑
               กด 1 ครั้ง ✓
```

### State Machine

```
[IDLE]
   |
   | กด (press)
   ↓
[PRESSED] ←————————————————————————+
   |                                |
   | กดนานกว่า 800ms                 |
   ↓                                |
[LONG_PRESS fired]                  |
   |                                |
   | ปล่อย                          |
   ↓                                |
[RELEASED] ——กดอีกใน 300ms——> [PRESSED] (click_count=2)
   |
   | เกิน 300ms แล้วไม่กด
   ↓
[IDLE] (single click complete)
```

---

## การใช้งานพื้นฐาน

### ขั้นตอนที่ 1: Include และประกาศตัวแปร

```c
#include "SimpleHAL.h"
#include "Button.h"

Button_Instance btn;  // ประกาศ global หรือ static
```

### ขั้นตอนที่ 2: Init และตั้ง Callback

```c
// ประกาศ callback functions
void on_btn_press(void) {
    printf("กดปุ่ม!\r\n");
}

void on_btn_long_press(void) {
    printf("กดค้าง!\r\n");
}

int main(void) {
    SystemCoreClockUpdate();
    Timer_Init();

    // Init ปุ่มบน PC4, Active LOW (ต่อระหว่าง pin และ GND)
    Button_Init(&btn, PC4, BUTTON_ACTIVE_LOW);

    // ตั้ง callback (ใส่ NULL สำหรับ event ที่ไม่ต้องการ)
    Button_SetCallback(&btn,
        on_btn_press,    // on_press
        NULL,            // on_release
        on_btn_long_press, // on_long_press
        NULL             // on_double_click
    );

    while (1) {
        Button_Update(&btn);  // ⚠️ ต้องเรียกใน loop เสมอ!
        Delay_Ms(1);          // รอ 1ms ระหว่าง update
    }
}
```

---

## การใช้งานขั้นสูง

### ตั้งค่า Timing เอง

```c
Button_Init(&btn, PC4, BUTTON_ACTIVE_LOW);

// ปรับ debounce เป็น 30ms (สำหรับปุ่มที่ bounce มาก)
Button_SetDebounce(&btn, 30);

// ปรับ long press เป็น 1.5 วินาที
Button_SetLongPressTime(&btn, 1500);

// ปรับ double click window เป็น 400ms
Button_SetDoubleClickTime(&btn, 400);
```

### Double Click Detection

```c
void on_single_press(void) {
    printf("คลิกครั้งเดียว\r\n");
}

void on_double_click(void) {
    printf("ดับเบิ้ลคลิก!\r\n");
}

// ตั้ง callback
Button_SetCallback(&btn, on_single_press, NULL, NULL, on_double_click);
```

> 💡 **หมายเหตุ**: ถ้าต้องการตรวจจับทั้ง single และ double click พร้อมกัน  
> single click จะถูกตรวจจับตอนปล่อยปุ่ม, double click จะตามมาถ้ากดอีกภายใน 300ms

### ตรวจสอบว่ากดค้างกี่วินาที

```c
while (1) {
    Button_Update(&btn);

    // วัดเวลาค้างแบบ realtime
    uint32_t hold_ms = Button_GetPressDurationMs(&btn);
    if (hold_ms >= 3000) {
        printf("กดค้างเกิน 3 วินาที\r\n");
        // ทำ action เช่น reset setting
    }

    // หลังปล่อยปุ่ม อ่านเวลาที่กดครั้งล่าสุด
    if (Button_GetEvent(&btn) == BUTTON_EVENT_RELEASE) {
        uint32_t last_ms = Button_GetLastPressDurationMs(&btn);
        printf("กดค้างล่าสุด: %lu ms\r\n", (unsigned long)last_ms);
    }

    Delay_Ms(1);
}
```

### ตรวจจับกดติดกัน 3 ครั้ง / 4 ครั้ง

```c
void on_multi_click(uint8_t count) {
    if (count == 3) {
        printf("Triple click\r\n");
        // ทำ action A
    } else if (count == 4) {
        printf("Quad click\r\n");
        // ทำ action B
    } else {
        printf("Multi click: %u\r\n", count);
    }
}

int main(void) {
    Button_Init(&btn, PC4, BUTTON_ACTIVE_LOW);
    Button_SetDoubleClickTime(&btn, 350); // window สำหรับนับคลิกต่อเนื่อง

    Button_SetCallback(&btn, NULL, NULL, NULL, NULL);
    Button_SetAdvancedCallback(&btn, on_multi_click, NULL);

    while (1) {
        Button_Update(&btn);
        Delay_Ms(1);
    }
}
```

### กดค้างตั้งแต่เปิดสวิตช์ (Boot Hold)

```c
void on_boot_hold(void) {
    printf("เข้าโหมดตั้งค่า (boot hold)\r\n");
    // เช่น เข้า calibration mode
}

int main(void) {
    SystemCoreClockUpdate();
    Timer_Init();

    Button_Init(&btn, PC4, BUTTON_ACTIVE_LOW);
    Button_SetBootHoldTime(&btn, 2000); // กดค้าง 2 วินาทีตั้งแต่บูต
    Button_SetAdvancedCallback(&btn, NULL, on_boot_hold);

    while (1) {
        Button_Update(&btn);
        Delay_Ms(1);
    }
}
```

### ควบคุมหลาย ปุ่ม พร้อมกัน

```c
Button_Instance btn_ok;
Button_Instance btn_cancel;
Button_Instance btn_menu;

Button_Init(&btn_ok,     PC3, BUTTON_ACTIVE_LOW);
Button_Init(&btn_cancel, PC4, BUTTON_ACTIVE_LOW);
Button_Init(&btn_menu,   PC5, BUTTON_ACTIVE_LOW);

Button_SetCallback(&btn_ok,     on_ok,     NULL, on_ok_hold,    NULL);
Button_SetCallback(&btn_cancel, on_cancel, NULL, NULL,          NULL);
Button_SetCallback(&btn_menu,   on_menu,   NULL, on_menu_hold,  NULL);

while (1) {
    // อัพเดตทุกปุ่มใน loop เดียวกัน
    Button_Update(&btn_ok);
    Button_Update(&btn_cancel);
    Button_Update(&btn_menu);
    Delay_Ms(1);
}
```

### Polling Mode (ไม่ใช้ Callback)

```c
Button_Init(&btn, PC4, BUTTON_ACTIVE_LOW);

while (1) {
    Button_Update(&btn);

    // ตรวจสอบ event แบบ polling
    Button_Event ev = Button_GetEvent(&btn);

    switch (ev) {
        case BUTTON_EVENT_PRESS:
            printf("กด\r\n");
            break;
        case BUTTON_EVENT_LONG_PRESS:
            printf("กดค้าง\r\n");
            break;
        case BUTTON_EVENT_DOUBLE_CLICK:
            printf("ดับเบิ้ลคลิก\r\n");
            break;
        case BUTTON_EVENT_RELEASE:
            printf("ปล่อย\r\n");
            break;
        default:
            break;
    }

    Delay_Ms(1);
}
```

### ตรวจสอบการกดค้างเพื่อ Repeat Action

```c
void on_press(void) {
    // ทำอะไรสักอย่างเมื่อกดครั้งแรก
}

// ใน main loop: ตรวจสอบว่ากดค้างอยู่
while (1) {
    Button_Update(&btn);

    // ถ้ากดค้างอยู่ให้ทำงานซ้ำทุก 200ms
    if (Button_IsPressed(&btn)) {
        static uint32_t last_repeat = 0;
        if (Get_CurrentMs() - last_repeat >= 200) {
            last_repeat = Get_CurrentMs();
            printf("กดค้าง - repeat\r\n");
        }
    }

    Delay_Ms(1);
}
```

### Menu Navigation ด้วย 3 ปุ่ม

```c
Button_Instance btn_up;
Button_Instance btn_down;
Button_Instance btn_enter;

int menu_index = 0;
const char* menu_items[] = {"Option 1", "Option 2", "Option 3", "Option 4"};
const int   menu_count   = 4;

void on_up(void) {
    menu_index = (menu_index - 1 + menu_count) % menu_count;
    printf("▲ %s\r\n", menu_items[menu_index]);
}

void on_down(void) {
    menu_index = (menu_index + 1) % menu_count;
    printf("▼ %s\r\n", menu_items[menu_index]);
}

void on_enter(void) {
    printf("✓ เลือก: %s\r\n", menu_items[menu_index]);
}

// ใน main()
Button_Init(&btn_up,    PC3, BUTTON_ACTIVE_LOW);
Button_Init(&btn_down,  PC4, BUTTON_ACTIVE_LOW);
Button_Init(&btn_enter, PC5, BUTTON_ACTIVE_LOW);

Button_SetCallback(&btn_up,    on_up,    NULL, NULL, NULL);
Button_SetCallback(&btn_down,  on_down,  NULL, NULL, NULL);
Button_SetCallback(&btn_enter, on_enter, NULL, NULL, NULL);

while (1) {
    Button_Update(&btn_up);
    Button_Update(&btn_down);
    Button_Update(&btn_enter);
    Delay_Ms(1);
}
```

---

## Polling Mode vs Callback Mode

### Callback Mode (แนะนำสำหรับ event-driven)

```c
// ข้อดี: โค้ดสะอาด, แยก logic ออกเป็น function
// ข้อเสีย: ไม่ควรทำงานหนักใน callback

void on_press(void) {
    led_toggle();   // ทำงานเร็วๆ ได้
}

Button_SetCallback(&btn, on_press, NULL, NULL, NULL);
```

### Polling Mode (เหมาะกับ state machine)

```c
// ข้อดี: ควบคุม flow ได้ชัดเจน
// ข้อเสีย: ต้องจัดการ state เอง

Button_Event ev = Button_GetEvent(&btn);
if (ev == BUTTON_EVENT_PRESS) {
    // จัดการใน main loop โดยตรง
}
```

---

## Troubleshooting

### ปัญหา: กดปุ่มครั้งเดียวแต่ callback ถูกเรียกหลายครั้ง

| สาเหตุ | วิธีแก้ |
|--------|---------|
| Debounce ไม่พอ | เพิ่ม debounce เป็น 30-50ms |
| ไม่ได้เรียก `Button_Update()` ต่อเนื่อง | ตรวจสอบว่า loop ไม่ถูก block |
| ปุ่มคุณภาพต่ำ bounce มาก | เพิ่ม capacitor 100nF ระหว่าง pin และ GND |

### ปัญหา: Long Press ไม่ทำงาน

| สาเหตุ | วิธีแก้ |
|--------|---------|
| ลืมตั้ง callback | เช็คว่าส่ง `on_long_press` ไม่ใช่ `NULL` |
| `Delay_Ms()` ใน loop ใหญ่เกินไป | ลด delay เป็น ≤5ms |
| `long_press_ms` ตั้งไว้นานเกิน | ลองปรับลงดูก่อน |

### ปัญหา: Double Click ไม่ทำงาน

| สาเหตุ | วิธีแก้ |
|--------|---------|
| กดช้าเกินไป | เพิ่ม `double_click_ms` เป็น 400-500ms |
| กดเพียงครั้งเดียว | ต้องกดปล่อย แล้วกดอีกครั้งภายใน window |
| Long press ถูก fire ก่อน | Double click ไม่ทำงานถ้ากดค้างนานเกิน `long_press_ms` |

---

## API Reference

### `Button_Init(btn, pin, active_level)`
เริ่มต้น Button พร้อมตั้งค่า GPIO

| Parameter | Type | คำอธิบาย |
|-----------|------|----------|
| `btn` | `Button_Instance*` | ตัวแปร instance |
| `pin` | `uint8_t` | GPIO pin เช่น `PC4` |
| `active_level` | `Button_ActiveLevel` | `BUTTON_ACTIVE_LOW` หรือ `BUTTON_ACTIVE_HIGH` |

---

### `Button_Update(btn)`
อัพเดตสถานะ — **ต้องเรียกใน main loop ทุก iteration** พร้อม `Delay_Ms(1)`

---

### `Button_SetCallback(btn, on_press, on_release, on_long_press, on_double_click)`
ตั้ง callback functions — ใส่ `NULL` สำหรับ event ที่ไม่ต้องการ

---

### `Button_SetAdvancedCallback(btn, on_multi_click, on_boot_hold)`
ตั้ง callback ขั้นสูง

- `on_multi_click(count)` จะถูกเรียกเมื่อกดติดกันตั้งแต่ 3 ครั้งขึ้นไป
- `on_boot_hold()` จะถูกเรียกเมื่อกดค้างตั้งแต่เปิดระบบเกิน `boot_hold_ms`

---

### `Button_IsPressed(btn)` → `bool`
ตรวจสอบว่ากดอยู่หรือไม่ (non-blocking, ใช้ใน loop)

---

### `Button_GetEvent(btn)` → `Button_Event`
ดึง event ล่าสุดแล้วล้าง (สำหรับ polling mode)

---

### `Button_SetDebounce(btn, ms)`
ตั้ง debounce time — default: 20ms

---

### `Button_SetLongPressTime(btn, ms)`
ตั้ง long press threshold — default: 800ms

---

### `Button_SetDoubleClickTime(btn, ms)`
ตั้ง double click window — default: 300ms

---

### `Button_SetBootHoldTime(btn, ms)`
ตั้งเวลาค้างตั้งแต่เปิดระบบเพื่อ trigger boot-hold event — default: 2000ms

---

### `Button_GetPressDurationMs(btn)` → `uint32_t`
อ่านระยะเวลาที่กดค้างอยู่ในขณะนี้ (ms)

---

### `Button_GetLastPressDurationMs(btn)` → `uint32_t`
อ่านระยะเวลาการกดครั้งล่าสุด (ms)

---

### `Button_GetLastClickCount(btn)` → `uint8_t`
อ่านจำนวนคลิกล่าสุดที่ finalize แล้ว

---

### `Button_Reset(btn)`
ล้าง state และ event ทั้งหมด

---

### `Button_EventStr(event)` → `const char*`
แปลง event เป็น string เช่น `"PRESS"`, `"LONG_PRESS"`

---

### Button_Event Values

| ค่า | ความหมาย |
|-----|----------|
| `BUTTON_EVENT_NONE` | ไม่มี event |
| `BUTTON_EVENT_PRESS` | กดปุ่ม (leading edge) |
| `BUTTON_EVENT_RELEASE` | ปล่อยปุ่ม (trailing edge) |
| `BUTTON_EVENT_LONG_PRESS` | กดค้างเกิน threshold |
| `BUTTON_EVENT_DOUBLE_CLICK` | กดสองครั้งรวดเร็ว |
| `BUTTON_EVENT_MULTI_CLICK` | กดติดกันตั้งแต่ 3 ครั้งขึ้นไป |
| `BUTTON_EVENT_BOOT_HOLD` | กดค้างตั้งแต่เปิดระบบ |

---

> **หมายเหตุ:** ไลบรารีนี้ถูกพอร์ตจากโปรเจกต์ CH32V003 ผ่าน Compatibility Shim  
> CH57x (CH570D) มีเฉพาะ PORTA (PA0–PA15) — ต้องปรับ pin mapping จาก PCx/PDx เป็น PAx  
> ดูรายละเอียดได้ที่ [compat_shim.md](../../SimpleHAL/docs/compat_shim.md)