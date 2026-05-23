/**
 * @file KeyMatrix.c
 * @brief 4x4 Keypad Matrix Library Implementation
 * @version 1.0
 * @date 2026-04-29
 */

#include "KeyMatrix.h"

/* ========== Built-in Keymaps ========== */

const char KEYMATRIX_MAP_4X4[4][4] = {
    {'1', '2', '3', 'A'},
    {'4', '5', '6', 'B'},
    {'7', '8', '9', 'C'},
    {'*', '0', '#', 'D'}
};

const char KEYMATRIX_MAP_4X3[4][3] = {
    {'1', '2', '3'},
    {'4', '5', '6'},
    {'7', '8', '9'},
    {'*', '0', '#'}
};

/* ========== Private: Scan one row ========== */

/**
 * @brief สแกน 1 row และคืน column ที่กด (หรือ -1 ถ้าไม่มี)
 *
 * @param kp  - instance ที่ผ่านการ init แล้ว
 * @param row - ดัชนีของ row ที่ต้องการสแกน (0-based)
 * @return column index ที่ตรวจพบ (0-based), -1 ถ้าไม่มี
 *
 * @note อัลกอริทึม Row Scan:
 *       ดึง row ที่ต้องการเป็น LOW, อีกทั้งหมดเป็น HIGH
 *       อ่าน column pins (INPUT_PULLUP) — LOW = กด
 *       Delay_Us(10) เพื่อรอ output stable
 */
static int8_t _scan_row(KeyMatrix_Instance* kp, uint8_t row) {
    /* ดึง row ที่ต้องการลง LOW, row อื่น HIGH */
    for (uint8_t r = 0; r < kp->num_rows; r++) {
        digitalWrite(kp->row_pins[r], (r == row) ? 0 : 1);
    }
    Delay_Us(10);  /* รอให้ output stable */

    /* อ่าน columns */
    for (uint8_t c = 0; c < kp->num_cols; c++) {
        if (digitalRead(kp->col_pins[c]) == 0) {
            return (int8_t)c;
        }
    }
    return -1;  /* ไม่มีปุ่มถูกกดใน row นี้ */
}

/**
 * @brief สแกนทุก row และคืน key ที่พบ (หรือ 0)
 *
 * @param kp - instance ที่ผ่านการ init แล้ว
 * @return character ของปุ่มจาก keymap, 0 ถ้าไม่มี
 *
 * @note วนลูป _scan_row() ทุก row ตั้งแต่ 0
 *       เจอปุ่ม → คืนทุก row เป็น HIGH แล้ว return keymap[row][col]
 *       ไม่เจอ → คืนทุก row เป็น HIGH, return 0
 */
static char _scan_all(KeyMatrix_Instance* kp) {
    for (uint8_t r = 0; r < kp->num_rows; r++) {
        int8_t col = _scan_row(kp, r);
        if (col >= 0 && col < (int8_t)kp->num_cols) {
            /* คืน row ทั้งหมดเป็น HIGH */
            for (uint8_t i = 0; i < kp->num_rows; i++) {
                digitalWrite(kp->row_pins[i], 1);
            }
            return kp->keymap[r][col];
        }
    }
    /* คืน row ทั้งหมดเป็น HIGH */
    for (uint8_t i = 0; i < kp->num_rows; i++) {
        digitalWrite(kp->row_pins[i], 1);
    }
    return 0;
}

/* ========== Public Functions ========== */

/**
 * @brief เริ่มต้น KeyMatrix
 *
 * @param kp       ตัวแปร instance (NULL → return KEYMATRIX_ERROR_PARAM)
 * @param rows     array ของ GPIO pins สำหรับ rows (NULL → error)
 * @param num_rows จำนวน rows (1-KEYMATRIX_MAX_ROWS)
 * @param cols     array ของ GPIO pins สำหรับ cols (NULL → error)
 * @param num_cols จำนวน cols (1-KEYMATRIX_MAX_COLS)
 * @param keymap   2D array ขนาด num_rows × KEYMATRIX_MAX_COLS (NULL → error)
 * @return KEYMATRIX_OK สำเร็จ, KEYMATRIX_ERROR_PARAM ถ้าพารามิเตอร์ไม่ถูกต้อง
 *
 * @note rows ถูกตั้งเป็น OUTPUT (idle HIGH)
 *       cols ถูกตั้งเป็น INPUT_PULLUP
 *       keymap เก็บ pointer (ไม่ copy) — ต้องคงอยู่ตลอดอายุการใช้งาน
 */
KeyMatrix_Status KeyMatrix_Init(KeyMatrix_Instance* kp,
                                 GPIO_Pin* rows, uint8_t num_rows,
                                 GPIO_Pin* cols, uint8_t num_cols,
                                 const char keymap[][KEYMATRIX_MAX_COLS]) {
    if (kp == NULL || rows == NULL || cols == NULL || keymap == NULL)
        return KEYMATRIX_ERROR_PARAM;
    if (num_rows == 0 || num_rows > KEYMATRIX_MAX_ROWS) return KEYMATRIX_ERROR_PARAM;
    if (num_cols == 0 || num_cols > KEYMATRIX_MAX_COLS) return KEYMATRIX_ERROR_PARAM;

    kp->num_rows      = num_rows;
    kp->num_cols      = num_cols;
    kp->keymap        = keymap;
    kp->last_key      = 0;
    kp->press_start_ms = 0;
    kp->is_pressed    = 0;
    kp->initialized   = 0;

    for (uint8_t r = 0; r < num_rows; r++) {
        kp->row_pins[r] = rows[r];
        pinMode(rows[r], PIN_MODE_OUTPUT);
        digitalWrite(rows[r], 1);  /* idle HIGH */
    }
    for (uint8_t c = 0; c < num_cols; c++) {
        kp->col_pins[c] = cols[c];
        pinMode(cols[c], PIN_MODE_INPUT_PULLUP);
    }

    kp->initialized = 1;
    return KEYMATRIX_OK;
}

/**
 * @brief สแกนและคืนปุ่มที่กด (non-blocking, มี debounce)
 *
 * @param kp - instance ที่ผ่านการ init แล้ว (NULL หรือ !initialized → return 0)
 * @return ตัวอักษรของปุ่มที่กด หรือ 0 ถ้าไม่มีปุ่ม หรือยังอยู่ในช่วง debounce
 *
 * @note เรียก _scan_all() ภายใน
 *       กดใหม่ → เริ่ม计时 debounce (KEYMATRIX_DEBOUNCE_MS)
 *       คืนเฉพาะตอนกดใหม่ (rising edge) ไม่ซ้ำขณะกดค้าง
 *       ควรเรียกทุก 10-20ms ใน main loop
 */
char KeyMatrix_GetKey(KeyMatrix_Instance* kp) {
    if (kp == NULL || !kp->initialized) return 0;

    char current = _scan_all(kp);

    if (current != 0) {
        /* มีปุ่มถูกกด */
        if (!kp->is_pressed) {
            /* กดใหม่ — เริ่ม debounce timer */
            kp->is_pressed     = 1;
            kp->press_start_ms = Get_CurrentMs();
            kp->last_key       = current;
            return 0;  /* รอ debounce ก่อน */
        }

        /* ตรวจ debounce */
        if ((Get_CurrentMs() - kp->press_start_ms) >= KEYMATRIX_DEBOUNCE_MS) {
            if (kp->last_key == current) {
                char key = kp->last_key;
                kp->last_key = 0;  /* ป้องกัน return ซ้ำ */
                return key;
            }
        }
    } else {
        /* ไม่มีปุ่มถูกกด */
        kp->is_pressed = 0;
        kp->last_key   = 0;
    }

    return 0;
}

/**
 * @brief ตรวจสอบว่าปุ่มใดกำลังถูกกดอยู่ตอนนี้ (ไม่ใช้ debounce)
 *
 * @param kp - instance ที่ผ่านการ init แล้ว (NULL หรือ !initialized → return 0)
 * @return ตัวอักษรของปุ่มที่กดอยู่ หรือ 0 ถ้าไม่มี
 *
 * @note เรียก _scan_all() โดยตรง, return ทันที
 *       ต่างจาก KeyMatrix_GetKey — คืนค่าตลอดขณะกดค้าง
 */
char KeyMatrix_GetCurrentKey(KeyMatrix_Instance* kp) {
    if (kp == NULL || !kp->initialized) return 0;
    return _scan_all(kp);
}

/**
 * @brief ตรวจสอบว่าปุ่มนี้กำลังถูก Long Press หรือเปล่า
 *
 * @param kp - instance ที่ผ่านการ init แล้ว (NULL หรือ !initialized → return 0)
 * @return ตัวอักษรของปุ่มที่ long press, 0 ถ้าไม่มี
 *
 * @note กดค้างนานกว่า KEYMATRIX_LONG_PRESS_MS
 *       ใช้กลไก is_pressed + press_start_ms เพื่อจับเวลา
 *       คืนครั้งเดียว (reset is_pressed หลัง detect)
 */
char KeyMatrix_GetLongPress(KeyMatrix_Instance* kp) {
    if (kp == NULL || !kp->initialized) return 0;

    char current = _scan_all(kp);
    if (current != 0) {
        if (!kp->is_pressed) {
            kp->is_pressed     = 1;
            kp->press_start_ms = Get_CurrentMs();
            kp->last_key       = current;
        } else if ((Get_CurrentMs() - kp->press_start_ms) >= KEYMATRIX_LONG_PRESS_MS) {
            /* Long press detected */
            kp->is_pressed = 0;  /* reset เพื่อ trigger ครั้งเดียว */
            return current;
        }
    } else {
        kp->is_pressed = 0;
        kp->last_key   = 0;
    }
    return 0;
}

/**
 * @brief รอจนมีปุ่มถูกกด (blocking)
 *
 * @param kp - instance ที่ผ่านการ init แล้ว (NULL หรือ !initialized → return 0)
 * @return ตัวอักษรของปุ่มที่กด
 *
 * @note วนลูป _scan_all() + Delay_Ms(20) รอจนเจอปุ่ม
 *       จากนั้นรอปล่อยปุ่ม (poll + Delay_Ms(10))
 *       debounce KEYMATRIX_DEBOUNCE_MS ก่อน return
 *       blocking — ไม่ควรเรียกจาก ISR
 */
char KeyMatrix_WaitKey(KeyMatrix_Instance* kp) {
    if (kp == NULL || !kp->initialized) return 0;

    char key;
    do {
        key = _scan_all(kp);
        Delay_Ms(20);
    } while (key == 0);

    /* รอปล่อยปุ่ม */
    while (_scan_all(kp) != 0) {
        Delay_Ms(10);
    }
    Delay_Ms(KEYMATRIX_DEBOUNCE_MS);
    return key;
}
