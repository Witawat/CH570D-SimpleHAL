# KeyMatrix Keypad Library

> **Library สำหรับใช้งาน Matrix Keypad (4x4, 4x3 ฯลฯ) บน CH57x**

## ภาพรวม

KeyMatrix ใช้หลักการ **Row-Column Scanning** เพื่ออ่านปุ่มจาก keypad matrix  
โดยใช้เพียง `(rows + cols)` GPIO pins สำหรับปุ่ม `(rows × cols)` ปุ่ม

---

## Hardware Setup

### 4x4 Keypad (16 ปุ่ม, 8 สาย)

```
Keypad 4x4       CH57x
ROW1 (pin 5) --> PD2 (OUTPUT)
ROW2 (pin 6) --> PD3 (OUTPUT)
ROW3 (pin 7) --> PD4 (OUTPUT)
ROW4 (pin 8) --> PD5 (OUTPUT)
COL1 (pin 1) <-- PC0 (INPUT_PULLUP)
COL2 (pin 2) <-- PC1 (INPUT_PULLUP)
COL3 (pin 3) <-- PC2 (INPUT_PULLUP)
COL4 (pin 4) <-- PC3 (INPUT_PULLUP)
```

### Layout ปุ่มมาตรฐาน

```
+---+---+---+---+
| 1 | 2 | 3 | A |
+---+---+---+---+
| 4 | 5 | 6 | B |
+---+---+---+---+
| 7 | 8 | 9 | C |
+---+---+---+---+
| * | 0 | # | D |
+---+---+---+---+
```

---

## การใช้งานพื้นฐาน

```c
#include "SimpleHAL.h"
#include "KeyMatrix.h"

KeyMatrix_Instance kp;

GPIO_Pin rows[] = { PIN_PD2, PIN_PD3, PIN_PD4, PIN_PD5 };
GPIO_Pin cols[] = { PIN_PC0, PIN_PC1, PIN_PC2, PIN_PC3 };

int main(void) {
    SystemCoreClockUpdate();
    Timer_Init();

    KeyMatrix_Init(&kp, rows, 4, cols, 4, KEYMATRIX_MAP_4X4);

    while (1) {
        char key = KeyMatrix_GetKey(&kp);
        if (key != 0) {
            printf("Pressed: %c\r\n", key);
        }
        Delay_Ms(20);
    }
}
```

---

## การใช้งานขั้นสูง

### รับ PIN Code 4 หลัก

```c
char pin[5] = {0};
printf("Enter PIN: ");
for (int i = 0; i < 4; i++) {
    pin[i] = KeyMatrix_WaitKey(&kp);
    printf("*");
}
printf("\r\n");

if (strcmp(pin, "1234") == 0) {
    printf("Correct!\r\n");
} else {
    printf("Wrong PIN!\r\n");
}
```

### Long Press

```c
while (1) {
    char key = KeyMatrix_GetKey(&kp);
    if (key) printf("Short press: %c\r\n", key);

    char lkey = KeyMatrix_GetLongPress(&kp);
    if (lkey) printf("Long press: %c\r\n", lkey);

    Delay_Ms(20);
}
```

### Custom Keymap

```c
// keymap ต้องมี KEYMATRIX_MAX_COLS (4) columns เสมอ
const char my_map[4][4] = {
    {'Q','W','E','R'},
    {'A','S','D','F'},
    {'Z','X','C','V'},
    {' ',',','.','!'}
};
KeyMatrix_Init(&kp, rows, 4, cols, 4, my_map);
```

---

## Troubleshooting

| ปัญหา | สาเหตุ | วิธีแก้ |
|-------|--------|---------|
| ไม่ตอบสนอง | Row/Col สับสน | ตรวจว่า rows=OUTPUT, cols=INPUT_PULLUP |
| กดปุ่มเดียวได้หลายตัว | Debounce สั้น | เพิ่ม `KEYMATRIX_DEBOUNCE_MS` เป็น 30-50 |
| keymap ผิดตัว | map ไม่ตรงกับ keypad | ทดสอบ scan ดิบๆ ก่อนใช้ keymap |

---

## API Reference

| Function | คำอธิบาย |
|----------|----------|
| `KeyMatrix_Init(kp, rows, nr, cols, nc, map)` | Init keypad |
| `KeyMatrix_GetKey(kp)` | อ่านปุ่ม (debounced, single fire) |
| `KeyMatrix_GetCurrentKey(kp)` | อ่านปุ่มที่กดอยู่ทันที (ไม่ debounce) |
| `KeyMatrix_GetLongPress(kp)` | ตรวจ long press (>800ms) |
| `KeyMatrix_WaitKey(kp)` | รอปุ่ม (blocking) |

---

> **หมายเหตุ:** ไลบรารีนี้ถูกพอร์ตจากโปรเจกต์ CH32V003 ผ่าน Compatibility Shim  
> CH57x (CH570D) มีเฉพาะ PORTA (PA0–PA15) — ต้องปรับ pin mapping จาก PCx/PDx เป็น PAx  
> ดูรายละเอียดได้ที่ [compat_shim.md](../../SimpleHAL/docs/compat_shim.md)