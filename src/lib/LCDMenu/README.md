# LCD Menu Library

> **Library สำหรับสร้างเมนูบน LCD1602/LCD2004 ควบคุมด้วยปุ่ม 4 ปุ่ม (UP / DOWN / ENTER / BACK)**

## 📋 สารบัญ

1. [ภาพรวม](#ภาพรวม)
2. [คุณสมบัติ](#คุณสมบัติ)
3. [Hardware Setup](#hardware-setup)
4. [หลักการทำงาน](#หลักการทำงาน)
5. [การใช้งานพื้นฐาน](#การใช้งานพื้นฐาน)
6. [Menu Item Types](#menu-item-types)
7. [การใช้งานขั้นสูง](#การใช้งานขั้นสูง)
8. [Troubleshooting](#troubleshooting)
9. [API Reference](#api-reference)

---

## ภาพรวม

LCDMenu เป็น library ที่ต่อยอดจาก `LCD1602_I2C` และ `Button` เพื่อให้สามารถสร้างเมนูแบบหลายระดับ
บน LCD character display ได้ง่าย โดยมีคุณสมบัติ:

- **Menu Tree**: สร้างเมนูแบบต้นไม้ รองรับ sub-menu ซ้อนกันหลายระดับ
- **Page-based Display**: แสดงรายการเต็มจอตามขนาด LCD (2 items/หน้าสำหรับ 16x2, 4 items/หน้าสำหรับ 20x4)
- **4-Button Navigation**: UP/DOWN เลื่อน, ENTER เลือก/ยืนยัน, BACK ย้อนกลับ
- **Edit Mode**: ปรับค่า ON/OFF (toggle) และตัวเลข (value) ได้โดยตรงจากเมนู

---

## คุณสมบัติ

- ✅ รองรับเมนูย่อยซ้อนกันหลายระดับ (sub-menu)
- ✅ Menu item 4 ประเภท: Sub-menu, Callback, Toggle, Value
- ✅ Edit mode สำหรับ Toggle (ON/OFF) และ Value (ปรับค่า)
- ✅ Page scrolling อัตโนมัติเมื่อ items เกินจำนวนแถว
- ✅ Custom cursor `>` (ลูกศรขวา) — ใช้ CGRAM
- ✅ ปุ่ม 4 ปุ่ม: UP / DOWN / ENTER / BACK
- ✅ รองรับทั้ง LCD 16x2 และ LCD 20x4 (แสดงผลอัตโนมัติตามขนาด)
- ✅ Static memory pool — ไม่ใช้ malloc
- ✅ Callback per menu item
- ✅ Title bar ตรงกลางบรรทัดแรก
- ✅ Wrap-around navigation (UP ที่ item แรก → ไป item สุดท้าย)

---

## Hardware Setup

### อุปกรณ์ที่ต้องใช้
- CH57x MCU
- LCD1602 หรือ LCD2004 แบบ I2C (PCF8574)
- Push button 4 ตัว
- Resistor 10kΩ (ถ้าไม่ใช้ internal pull-up)
- Breadboard + สายไฟ

### Wiring Diagram

```
CH57x                   LCD1602 I2C
+--------+                 +----------+
|        |                 |          |
|   PC1  |----[SDA]--------| SDA      |
|   PC2  |----[SCL]--------| SCL      |
|  3.3V  |-----------------| VCC      |
|   GND  |-----------------| GND      |
+--------+                 +----------+

Pull-up 4.7kΩ ระหว่าง SDA/SCL กับ 3.3V

CH57x                   Push Buttons (Active LOW)
+--------+
|        |          ___
|   PC0  |----+----|___|----+---- GND
|             |    BTN_UP   |
|  3.3V --[10kΩ]-----------+  (internal pull-up)
|        |
|   PC3  |----+----[BTN_DOWN]---- GND
|   PC4  |----+----[BTN_ENTER]--- GND
|   PC5  |----+----[BTN_BACK]---- GND
+--------+
```

**Pin Configuration (ตัวอย่าง):**
| ปุ่ม | GPIO Pin | หมายเหตุ |
|------|----------|----------|
| UP   | PC0      | ปรับได้ตามต้องการ |
| DOWN | PC3      | ปรับได้ตามต้องการ |
| ENTER| PC4      | ปรับได้ตามต้องการ |
| BACK | PC5      | ปรับได้ตามต้องการ |

> **หมายเหตุ:** ใช้ Active LOW (internal pull-up) เพื่อลดจำนวนอุปกรณ์ภายนอก

---

## หลักการทำงาน

### State Machine

```
             ┌──────────────────────────┐
             │     NORMAL MODE          │
             │  (เลื่อน cursor ด้วย      │
             │   UP/DOWN, เลือกด้วย     │
             │   ENTER, กลับด้วย BACK)  │
             └───────┬──────────────────┘
                     │
          ┌──────────┼──────────┐
          ▼          ▼          ▼
    ┌─────────┐ ┌────────┐ ┌──────────┐
    │SUBMENU  │ │CALLBACK│ │TOGGLE/   │
    │→ เข้า    │ │→ fire  │ │VALUE     │
    │  เมนูย่อย│ │callback│ │→ edit    │
    └─────────┘ └────────┘ └────┬─────┘
                                │
                          ┌─────▼─────┐
                          │ EDIT MODE │
                          │ UP/DOWN   │
                          │ ปรับค่า   │
                          │ ENTER=ยืนยัน│
                          │ BACK=ยกเลิก│
                          └───────────┘
```

### Display Layout

```
16x2 Layout:                 20x4 Layout:
┌────────────────┐           ┌──────────────────────┐
│   TITLE BAR    │ (row 0)   │      TITLE BAR       │ (row 0)
├────────────────┤           ├──────────────────────┤
│ > Item 1       │ (row 1)   │ > Item 1     [ON]    │ (row 1)
│   Item 2  >    │           │   Item 2             │ (row 2)
└────────────────┘           │   Item 3     50      │ (row 3)
                             │   Item 4  >          │
                             └──────────────────────┘
```

- `>` ด้านซ้าย = cursor (item ที่เลือก)
- `>` ด้านขวา = sub-menu (มีเมนูย่อย)
- `[ON]`/`[OFF]` = toggle item
- ตัวเลข = value item
- `<val>` ใน edit mode = กำลังแก้ไขค่า

---

## การใช้งานพื้นฐาน

### Step 1: Initialize Hardware

```c
#include "SimpleHAL.h"
#include "Lib/LCD1602_I2C/lcd1602_i2c.h"
#include "Lib/Button/Button.h"
#include "Lib/LCDMenu/lcdmenu.h"

LCD1602_Handle lcd;
Button_Instance btn_up, btn_down, btn_enter, btn_back;
LCDMenu_Handle menu;

int main(void) {
    SystemCoreClockUpdate();
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1);
    Timer_Init();
    I2C_SimpleInit(I2C_100KHZ, I2C_PINS_DEFAULT);

    LCD_Init(&lcd, 0x27, LCD_16x2);
    LCD_Backlight(&lcd, 1);

    Button_Init(&btn_up,    PC0, BUTTON_ACTIVE_LOW);
    Button_Init(&btn_down,  PC3, BUTTON_ACTIVE_LOW);
    Button_Init(&btn_enter, PC4, BUTTON_ACTIVE_LOW);
    Button_Init(&btn_back,  PC5, BUTTON_ACTIVE_LOW);
```

### Step 2: Create Menu Structure

```c
    LCDMenu_Init(&menu, &lcd, &btn_up, &btn_down, &btn_enter, &btn_back);
    LCDMenu_CreateRoot(&menu, "Main Menu");

    // Sub-menu: Settings
    LCDMenu_Item* settings = LCDMenu_AddSubMenu(&menu, menu.root, "Settings", NULL);

    static bool backlight_on = true;
    LCDMenu_AddToggle(&menu, settings, "Backlight", &backlight_on, NULL);

    static int32_t volume = 50;
    LCDMenu_AddValue(&menu, settings, "Volume", &volume, 0, 100, 5, NULL);

    // Callback item
    void say_hello(void) {
        LCD_Clear(&lcd);
        LCD_Print(&lcd, "Hello!");
        Delay_Ms(1000);
    }
    LCDMenu_AddCallback(&menu, menu.root, "Say Hello", say_hello);

    // Sub-menu: About
    LCDMenu_Item* about = LCDMenu_AddSubMenu(&menu, menu.root, "About", NULL);
    LCDMenu_AddCallback(&menu, about, "Version 1.0", NULL);
```

### Step 3: Main Loop

```c
    LCDMenu_Draw(&menu);

    while (1) {
        Button_Update(&btn_up);
        Button_Update(&btn_down);
        Button_Update(&btn_enter);
        Button_Update(&btn_back);
        LCDMenu_Update(&menu);
        Delay_Ms(5);
    }
}
```

---

## Menu Item Types

| Type | ฟังก์ชันสร้าง | ลักษณะ | การกด ENTER |
|------|--------------|--------|-------------|
| **Sub-menu** | `LCDMenu_AddSubMenu()` | มี `>` ต่อท้าย | เข้าเมนูย่อย |
| **Callback** | `LCDMenu_AddCallback()` | ข้อความอย่างเดียว | เรียก callback function |
| **Toggle** | `LCDMenu_AddToggle()` | แสดง `[ON]`/`[OFF]` | สลับค่า ON/OFF ทันที |
| **Value** | `LCDMenu_AddValue()` | แสดงตัวเลข | เข้า edit mode |

### Sub-menu

เมนูที่มี children สำหรับนำทางลึกขึ้นไปอีกชั้น

```c
LCDMenu_Item* network = LCDMenu_AddSubMenu(&menu, settings, "Network", NULL);
LCDMenu_AddToggle(&menu, network, "WiFi", &wifi_on, NULL);
LCDMenu_AddValue(&menu, network, "Channel", &channel, 1, 13, 1, NULL);
```

### Callback

 item ที่ทำงานทันทีเมื่อกด ENTER (ไม่เข้า sub-menu)

```c
void reset_device(void) {
    NVIC_SystemReset();
}
LCDMenu_AddCallback(&menu, settings, "Reset Device", reset_device);
```

### Toggle

สำหรับเปิด/ปิดฟีเจอร์

```c
static bool led_enabled = true;

void on_led_changed(void) {
    if (led_enabled) {
        GPIO_WriteBit(GPIOC, GPIO_Pin_0, SET);
    } else {
        GPIO_WriteBit(GPIOC, GPIO_Pin_0, RESET);
    }
}

LCDMenu_AddToggle(&menu, settings, "LED", &led_enabled, on_led_changed);
```

### Value

สำหรับปรับค่าตัวเลข — กด ENTER เพื่อเข้า edit mode, UP/DOWN ปรับค่า, ENTER ยืนยัน, BACK ยกเลิก

```c
static int32_t brightness = 80;

void on_brightness_changed(void) {
    // อ่านค่าใหม่จาก brightness ได้เลย (ถูกอัปเดตแล้วหลัง ENTER)
    set_pwm(brightness);
}

LCDMenu_AddValue(&menu, settings, "Brightness", &brightness, 0, 100, 10, on_brightness_changed);
```

---

## การใช้งานขั้นสูง

### Callback ใน Sub-menu

callback ของ sub-menu จะถูกเรียกเมื่อเข้าเมนูนั้นครั้งแรก:

```c
void on_enter_settings(void) {
    // โหลดค่าจาก EEPROM ก่อนแสดง
    load_settings_from_eeprom();
}

LCDMenu_Item* settings = LCDMenu_AddSubMenu(&menu, menu.root, "Settings", on_enter_settings);
```

### Force Redraw จากภายนอก

ถ้ามีการเปลี่ยนค่าจาก interrupt หรือ timer callback ให้เรียก `LCDMenu_Draw()` เพื่ออัปเดตจอ:

```c
void TIM2_IRQHandler(void) {
    // อัปเดตค่า sensor...
    sensor_value = read_sensor();
    LCDMenu_Draw(&menu);  // force redraw
}
```

### นำทางด้วยโปรแกรม (GoTo)

```c
// กระโดดไปยังเมนู Settings โดยตรง
LCDMenu_GoTo(&menu, settings_menu);

// กลับไป root
LCDMenu_Reset(&menu);
```

### ใช้กับ LCD 20x4

```c
LCD_Init(&lcd, 0x27, LCD_20x4);
// LCDMenu จะปรับจำนวนแถวอัตโนมัติ (แสดง 4 items/หน้า)
```

---

## Troubleshooting

### ปัญหา: ปุ่มไม่ตอบสนอง

**สาเหตุ:** ลืมเรียก `Button_Update()` สำหรับทุกปุ่มก่อน `LCDMenu_Update()`

**วิธีแก้:**
```c
while (1) {
    Button_Update(&btn_up);    // ต้องเรียกทุกปุ่ม!
    Button_Update(&btn_down);
    Button_Update(&btn_enter);
    Button_Update(&btn_back);
    LCDMenu_Update(&menu);
    Delay_Ms(5);
}
```

### ปัญหา: กดปุ่มแล้วเลื่อนหลายครั้ง

**สาเหตุ:** เรียก `LCDMenu_Update()` ใน loop ที่เร็วกว่า debounce (default 20ms)

**วิธีแก้:** เพิ่ม `Delay_Ms(5)` หรือมากกว่าใน main loop

### ปัญหา: LCD แสดงผลผิดพลาด / garbage characters

**สาเหตุ:** `LCDMenu_Init()` ล้มเหลวในการสร้าง custom character

**วิธีแก้:** ตรวจสอบว่าเรียก `LCD_Init()` และ `I2C_SimpleInit()` ก่อน `LCDMenu_Init()`

### ปัญหา: Menu items หาย / pool เต็ม

**สาเหตุ:** สร้าง menu items เกิน `LCDMENU_MAX_ITEMS` (default 32)

**วิธีแก้:** เพิ่ม `#define LCDMENU_MAX_ITEMS 64` ก่อน include `lcdmenu.h`

---

## API Reference

### Initialization

| Function | Signature |
|----------|-----------|
| `LCDMenu_Init` | `void LCDMenu_Init(LCDMenu_Handle* menu, LCD1602_Handle* lcd, Button_Instance* btn_up, Button_Instance* btn_down, Button_Instance* btn_enter, Button_Instance* btn_back)` |

### Menu Tree Construction

| Function | Signature |
|----------|-----------|
| `LCDMenu_CreateRoot` | `void LCDMenu_CreateRoot(LCDMenu_Handle* menu, const char* title)` |
| `LCDMenu_AddSubMenu` | `LCDMenu_Item* LCDMenu_AddSubMenu(LCDMenu_Handle* menu, LCDMenu_Item* parent, const char* name, void (*callback)(void))` |
| `LCDMenu_AddCallback` | `LCDMenu_Item* LCDMenu_AddCallback(LCDMenu_Handle* menu, LCDMenu_Item* parent, const char* name, void (*callback)(void))` |
| `LCDMenu_AddToggle` | `LCDMenu_Item* LCDMenu_AddToggle(LCDMenu_Handle* menu, LCDMenu_Item* parent, const char* name, bool* value, void (*callback)(void))` |
| `LCDMenu_AddValue` | `LCDMenu_Item* LCDMenu_AddValue(LCDMenu_Handle* menu, LCDMenu_Item* parent, const char* name, int32_t* value, int32_t min, int32_t max, int32_t step, void (*callback)(void))` |

### Runtime & Navigation

| Function | Signature |
|----------|-----------|
| `LCDMenu_Update` | `void LCDMenu_Update(LCDMenu_Handle* menu)` |
| `LCDMenu_Back` | `void LCDMenu_Back(LCDMenu_Handle* menu)` |
| `LCDMenu_GoTo` | `void LCDMenu_GoTo(LCDMenu_Handle* menu, LCDMenu_Item* target)` |

### Display & State

| Function | Signature |
|----------|-----------|
| `LCDMenu_Draw` | `void LCDMenu_Draw(LCDMenu_Handle* menu)` |
| `LCDMenu_Reset` | `void LCDMenu_Reset(LCDMenu_Handle* menu)` |
| `LCDMenu_IsEditing` | `bool LCDMenu_IsEditing(LCDMenu_Handle* menu)` |
| `LCDMenu_GetCurrentItem` | `LCDMenu_Item* LCDMenu_GetCurrentItem(LCDMenu_Handle* menu)` |

---

## ข้อควรระวัง

1. **Memory**: แต่ละ `LCDMenu_Item` ใช้ ~72 bytes (pool 32 items ≈ 2.3 KB) — ตรวจสอบให้แน่ใจว่า RAM เพียงพอก่อนเพิ่ม `LCDMENU_MAX_ITEMS`
2. **Button Pool**: ปุ่ม 4 ปุ่มใช้ 4 จาก 8 slots ใน Button pool (default) — ถ้าใช้ปุ่มร่วมกับ library อื่น อาจต้องเพิ่ม `BUTTON_MAX_INSTANCES`
3. **CGRAM**: ตำแหน่ง 0 ถูกใช้สำหรับ cursor `>` — ถ้าต้องการใช้ custom character อื่น ให้ใช้ location 1-7
4. **Callback Safety**: callback ถูกเรียกจากภายใน `LCDMenu_Update()` ซึ่งอยู่ใน main loop — ห้ามเรียก `Delay_Ms()` นานๆ หรือ blocking operation ใน callback
5. **Variable Lifetime**: `bool*` และ `int32_t*` ที่ส่งให้ Toggle/Value ต้องเป็น static/global — ห้ามใช้ local variable

---

> © 2026 CH57x Library Team

---

> **หมายเหตุ:** ไลบรารีนี้ถูกพอร์ตจากโปรเจกต์ CH32V003 ผ่าน Compatibility Shim  
> CH57x (CH570D) มีเฉพาะ PORTA (PA0–PA15) — ต้องปรับ pin mapping จาก PCx/PDx เป็น PAx  
> ดูรายละเอียดได้ที่ [compat_shim.md](../../SimpleHAL/docs/compat_shim.md)