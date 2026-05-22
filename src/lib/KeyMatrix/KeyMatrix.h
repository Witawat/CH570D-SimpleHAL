/**
 * @file KeyMatrix.h
 * @brief 4x4 Keypad Matrix Library สำหรับ CH57x
 * @version 1.0
 * @date 2026-04-29
 *
 * @details
 * Library สำหรับอ่านปุ่มจาก Keypad Matrix แบบ Row-Column Scanning
 * รองรับ keypad ขนาด 1x1 ถึง 4x4 (ปรับได้ผ่าน defines)
 *
 * **หลักการทำงาน (Row Scan):**
 * - Rows = OUTPUT — สแกนทีละ row โดยดึง LOW
 * - Cols = INPUT_PULLUP — อ่านค่า (LOW = กดปุ่ม)
 * - เมื่อ row[i] = LOW และ col[j] = LOW → ปุ่มที่ (i, j) ถูกกด
 *
 * **Hardware Connection (4x4 keypad):**
 * ```
 *   Keypad        CH57x
 *   ROW1 -------> GPIO (output)
 *   ROW2 -------> GPIO (output)
 *   ROW3 -------> GPIO (output)
 *   ROW4 -------> GPIO (output)
 *   COL1 <------- GPIO (input, pull-up)
 *   COL2 <------- GPIO (input, pull-up)
 *   COL3 <------- GPIO (input, pull-up)
 *   COL4 <------- GPIO (input, pull-up)
 * ```
 *
 * @example
 * #include "SimpleHAL.h"
 * #include "KeyMatrix.h"
 *
 * KeyMatrix_Instance kp;
 *
 * GPIO_Pin rows[] = { PIN_PD2, PIN_PD3, PIN_PD4, PIN_PD5 };
 * GPIO_Pin cols[] = { PIN_PC0, PIN_PC1, PIN_PC2, PIN_PC3 };
 * const char keymap[4][4] = {
 *     {'1','2','3','A'},
 *     {'4','5','6','B'},
 *     {'7','8','9','C'},
 *     {'*','0','#','D'}
 * };
 *
 * int main(void) {
 *     SystemCoreClockUpdate();
 *     Timer_Init();
 *     KeyMatrix_Init(&kp, rows, 4, cols, 4, keymap);
 *
 *     while (1) {
 *         char key = KeyMatrix_GetKey(&kp);
 *         if (key != 0) printf("Key: %c\r\n", key);
 *         Delay_Ms(20);
 *     }
 * }
 *
 * @author CH57x Library Team
 * @copyright MIT License
 */

#ifndef __KEYMATRIX_H
#define __KEYMATRIX_H

#ifdef __cplusplus
extern "C" {
#endif

#include "../../SimpleHAL/SimpleHAL.h"
#include <stdint.h>
#include <stdbool.h>

/* ========== Configuration ========== */

/** @brief จำนวน rows สูงสุด */
#ifndef KEYMATRIX_MAX_ROWS
#define KEYMATRIX_MAX_ROWS  4
#endif

/** @brief จำนวน cols สูงสุด */
#ifndef KEYMATRIX_MAX_COLS
#define KEYMATRIX_MAX_COLS  4
#endif

/** @brief Debounce time (ms) */
#ifndef KEYMATRIX_DEBOUNCE_MS
#define KEYMATRIX_DEBOUNCE_MS  20
#endif

/** @brief Long press threshold (ms) */
#ifndef KEYMATRIX_LONG_PRESS_MS
#define KEYMATRIX_LONG_PRESS_MS  800
#endif

/* ========== Built-in Keymaps ========== */

/** @brief Keymap มาตรฐาน 4x4 (1-9, *, 0, #, A-D) */
extern const char KEYMATRIX_MAP_4X4[4][4];

/** @brief Keymap 4x3 (เหมือนโทรศัพท์: 1-9, *, 0, #) */
extern const char KEYMATRIX_MAP_4X3[4][3];

/* ========== Type Definitions ========== */

/**
 * @brief สถานะการทำงาน
 */
typedef enum {
    KEYMATRIX_OK         = 0,
    KEYMATRIX_ERROR_PARAM = 1
} KeyMatrix_Status;

/**
 * @brief KeyMatrix Instance
 */
typedef struct {
    GPIO_Pin row_pins[KEYMATRIX_MAX_ROWS]; /**< GPIO pins สำหรับ rows */
    GPIO_Pin col_pins[KEYMATRIX_MAX_COLS]; /**< GPIO pins สำหรับ cols */
    uint8_t  num_rows;                     /**< จำนวน rows จริง */
    uint8_t  num_cols;                     /**< จำนวน cols จริง */
    const char (*keymap)[KEYMATRIX_MAX_COLS]; /**< pointer ไปยัง keymap 2D array */
    char     last_key;                     /**< ปุ่มที่กดล่าสุด */
    uint32_t press_start_ms;               /**< เวลาที่เริ่มกดปุ่ม */
    uint8_t  is_pressed;                   /**< 1 = ปุ่มกำลังถูกกด */
    uint8_t  initialized;                  /**< flag บอกว่า Init แล้ว */
} KeyMatrix_Instance;

/* ========== Function Prototypes ========== */

/**
 * @brief เริ่มต้น KeyMatrix
 * @param kp      ตัวแปร instance
 * @param rows    array ของ GPIO pins สำหรับ rows
 * @param num_rows จำนวน rows (1-KEYMATRIX_MAX_ROWS)
 * @param cols    array ของ GPIO pins สำหรับ cols
 * @param num_cols จำนวน cols (1-KEYMATRIX_MAX_COLS)
 * @param keymap  2D array ของ character แต่ละปุ่ม (num_rows × KEYMATRIX_MAX_COLS)
 * @return KEYMATRIX_OK หรือ error code
 *
 * @note rows = OUTPUT (ดึง LOW ทีละ row), cols = INPUT_PULLUP
 * @example
 * GPIO_Pin rows[] = { PIN_PD2, PIN_PD3, PIN_PD4, PIN_PD5 };
 * GPIO_Pin cols[] = { PIN_PC0, PIN_PC1, PIN_PC2, PIN_PC3 };
 * KeyMatrix_Init(&kp, rows, 4, cols, 4, KEYMATRIX_MAP_4X4);
 */
KeyMatrix_Status KeyMatrix_Init(KeyMatrix_Instance* kp,
                                 GPIO_Pin* rows, uint8_t num_rows,
                                 GPIO_Pin* cols, uint8_t num_cols,
                                 const char keymap[][KEYMATRIX_MAX_COLS]);

/**
 * @brief สแกนและคืนปุ่มที่กด (non-blocking, มี debounce)
 * @param kp ตัวแปร instance
 * @return ตัวอักษรของปุ่มที่กด หรือ 0 ถ้าไม่มีปุ่มถูกกด
 *
 * @note ควรเรียกทุก 10-20ms ใน main loop
 * @note คืนค่าเฉพาะตอนที่ปุ่มถูก "กดใหม่" (rising edge) ไม่ใช่ขณะกดค้าง
 * @example
 * char key = KeyMatrix_GetKey(&kp);
 * if (key) printf("Key: %c\r\n", key);
 */
char KeyMatrix_GetKey(KeyMatrix_Instance* kp);

/**
 * @brief ตรวจสอบว่าปุ่มใดกำลังถูกกดอยู่ตอนนี้ (ไม่ใช้ debounce)
 * @param kp ตัวแปร instance
 * @return ตัวอักษรของปุ่มที่กดอยู่ หรือ 0 ถ้าไม่มี
 *
 * @note ต่างจาก GetKey — คืนค่าตลอดขณะกดค้าง
 */
char KeyMatrix_GetCurrentKey(KeyMatrix_Instance* kp);

/**
 * @brief ตรวจสอบว่าปุ่มนี้กำลังถูก Long Press หรือเปล่า
 * @param kp ตัวแปร instance
 * @return ตัวอักษรของปุ่มที่ long press หรือ 0 ถ้าไม่มี
 *
 * @note Long press = กดค้างนานกว่า KEYMATRIX_LONG_PRESS_MS
 */
char KeyMatrix_GetLongPress(KeyMatrix_Instance* kp);

/**
 * @brief รอจนมีปุ่มถูกกด (blocking)
 * @param kp ตัวแปร instance
 * @return ตัวอักษรของปุ่มที่กด
 *
 * @example
 * printf("กด PIN: ");
 * for (int i = 0; i < 4; i++) {
 *     char c = KeyMatrix_WaitKey(&kp);
 *     printf("%c", c);
 * }
 */
char KeyMatrix_WaitKey(KeyMatrix_Instance* kp);

#ifdef __cplusplus
}
#endif

#endif /* __KEYMATRIX_H */
