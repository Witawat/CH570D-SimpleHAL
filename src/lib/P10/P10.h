/**
 * @file P10.h
 * @brief P10 LED Matrix Display Library สำหรับ CH57x series
 * @version 1.0
 * @date 2026-05-03
 *
 * @details
 * Library สำหรับขับ P10 LED Matrix Panels (สีเดี่ยว / สองสี / RGB)
 * ใช้ timer interrupt scan + bit-bang shift register protocol
 * รองรับ panel ขนาด 32x16 และ 16x16 แบบ 1 panel (On/Off ไม่มี grayscale)
 *
 * **คุณสมบัติ:**
 * - รองรับ P10 แบบสีเดี่ยว (1 data pin), สองสี (2 data pins), RGB (3 data pins)
 * - เลือก color mode ตอน compile เพื่อประหยัด RAM/Flash
 * - Timer interrupt scan (non-blocking, ~200Hz refresh)
 * - Configurable resolution (32x16 / 16x16 / ปรับเอง)
 * - Configurable bit order (MSB-first / LSB-first)
 * - SetPixel / Clear / Fill API
 * - Pin mapping ยืดหยุ่น (ใช้ GPIO pin ใดก็ได้)
 * - รองรับ CH57x series
 *
 * **Hardware Connection:**
 * ```
 * P10 Single Color (1/4 หรือ 1/16 scan):
 *
 *   CH57x              P10 Panel
 *   DATA (PC0)     -----> R (Red Data)
 *   CLK  (PC1)     -----> CLK
 *   LAT  (PC2)     -----> STB/LAT
 *   OE   (PC3)     -----> OE
 *   A    (PC4)     -----> A
 *   B    (PC5)     -----> B
 *   C    (PC6)     -----> C (ถ้า 8+ scan rows)
 *   D    (PC7)     -----> D (ถ้า 16 scan rows)
 *   GND            -----> GND
 *   5V             -----> VCC (ถ้า panel ใช้ 5V)
 * ```
 *
 * **P10 RGB (1/16 scan):**
 * ```
 *   CH57x              P10 Panel
 *   R    (PC0)     -----> Red Data
 *   G    (PC1)     -----> Green Data
 *   B    (PC2)     -----> Blue Data
 *   CLK  (PC3)     -----> CLK
 *   LAT  (PC4)     -----> STB/LAT
 *   OE   (PC5)     -----> OE
 *   A    (PC6)     -----> A
 *   B    (PC7)     -----> B
 *   C    (PD2)     -----> C
 *   D    (PD3)     -----> D
 *   GND            -----> GND
 * ```
 *
 * @note CH57x (CH570D) มี GPIO จำกัดเฉพาะ PORTA (~15 pins) → วางแผน pin mapping ให้ดี
 * @note ต้องมี Timer_Init() ก่อน (เรียกอัตโนมัติจาก SimpleDelay constructor)
 * @note ใช้ TIM2 สำหรับ scan interrupt (ห้ามใช้ TIM2 PWM พร้อมกัน)
 *
 * @example
 * #include "P10.h"
 *
 * int main(void) {
 *     SystemCoreClockUpdate();
 *     Timer_Init();
 *
 *     // กำหนดค่า P10 panel (สีแดงเดี่ยว 32x16, MSB-first)
 *     P10_Config cfg = P10_CONFIG_DEFAULT(32, 16);
 *     cfg.data_r_pin = PC0;   // Red data
 *     cfg.clk_pin    = PC1;   // Shift clock
 *     cfg.lat_pin    = PC2;   // Latch
 *     cfg.oe_pin     = PC3;   // Output enable
 *     cfg.row_a_pin  = PC4;   // Row A
 *     cfg.row_b_pin  = PC5;   // Row B
 *     cfg.row_c_pin  = PC6;   // Row C
 *     cfg.row_d_pin  = PC7;   // Row D
 *
 *     P10_Instance display;
 *     P10_Init(&display, &cfg);
 *
 *     // วาดพิกเซลที่ (0, 0), (1, 1), (31, 15)
 *     P10_SetPixel(&display, 0, 0, 1);
 *     P10_SetPixel(&display, 1, 1, 1);
 *     P10_SetPixel(&display, 31, 15, 1);
 *
 *     while(1) {
 *         // Timer ISR จัดการ scan อัตโนมัติ
 *     }
 * }
 *
 * @author CH57x Library Team
 * @copyright MIT License
 */

#ifndef __P10_H
#define __P10_H

#ifdef __cplusplus
extern "C" {
#endif

#include "../../SimpleHAL/SimpleHAL.h"
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

/* ==================================================================
 *  Color Mode Selection (เลือก 1 ค่า ก่อน compile)
 *  ==============================================================
 *  P10_SINGLE_COLOR — 1 data pin (R)  — buffer = W*H/8 bytes
 *  P10_DUAL_COLOR   — 2 data pins (R+G) — buffer = W*H/4 bytes
 *  P10_RGB          — 3 data pins (R+G+B) — buffer = 3*W*H/8 bytes
 * ================================================================== */

#ifndef P10_COLOR_MODE
#define P10_COLOR_MODE  P10_SINGLE_COLOR   /**< Default: single color */
#endif

#define P10_SINGLE_COLOR  0
#define P10_DUAL_COLOR    1
#define P10_RGB           2

/* ========== Bit Order ========== */

#define P10_MSB_FIRST  0   /**< Most Significant Bit first */
#define P10_LSB_FIRST  1   /**< Least Significant Bit first */

/* ========== Configuration Macros (overrideable) ========== */

#ifndef P10_MAX_PANELS
#define P10_MAX_PANELS  1  /**< Maximum number of P10 panels */
#endif

#ifndef P10_REFRESH_RATE
#define P10_REFRESH_RATE  200  /**< Panel refresh rate (Hz) */
#endif

#ifndef P10_OE_HOLD_US
#define P10_OE_HOLD_US   10  /**< OE enable hold time (µs) — ปรับความสว่าง */
#endif

/* ========== Default Config Macro ========== */

/**
 * @brief Macro สำหรับสร้าง P10_Config เริ่มต้น
 * @param w ความกว้าง (px) เช่น 32
 * @param h ความสูงรวม (px) เช่น 16
 *
 * ตั้งค่าเริ่มต้น: color_mode จาก P10_COLOR_MODE, MSB-first, refresh 200Hz
 * ยังต้องกำหนด pin mapping ก่อน Init
 */
#define P10_CONFIG_DEFAULT(w, h) {                         \
    .data_r_pin   = 0xFF,                                  \
    .data_g_pin   = 0xFF,                                  \
    .data_b_pin   = 0xFF,                                  \
    .clk_pin      = 0xFF,                                  \
    .lat_pin      = 0xFF,                                  \
    .oe_pin       = 0xFF,                                  \
    .row_a_pin    = 0xFF,                                  \
    .row_b_pin    = 0xFF,                                  \
    .row_c_pin    = 0xFF,                                  \
    .row_d_pin    = 0xFF,                                  \
    .width        = (w),                                   \
    .height       = (h),                                   \
    .rows         = (h),   /* full height = scan rows */   \
    .color_mode   = P10_COLOR_MODE,                        \
    .bit_order    = P10_MSB_FIRST,                         \
    .refresh_rate = P10_REFRESH_RATE                       \
}

/* ========== Color Mode Helpers ========== */

/** @brief จำนวน bit planes ตาม color mode */
#if (P10_COLOR_MODE == P10_SINGLE_COLOR)
    #define P10_NUM_PLANES  1
#elif (P10_COLOR_MODE == P10_DUAL_COLOR)
    #define P10_NUM_PLANES  2
#elif (P10_COLOR_MODE == P10_RGB)
    #define P10_NUM_PLANES  3
#else
    #error "Invalid P10_COLOR_MODE. Use P10_SINGLE_COLOR, P10_DUAL_COLOR, or P10_RGB."
#endif

/* ========== Type Definitions ========== */

/**
 * @brief P10 Panel Configuration
 */
typedef struct {
    /* ----- Data Pins ----- */
    uint8_t data_r_pin;   /**< Red data pin (หรือ สีเดียว) — ใช้ SimpleGPIO enum */
    uint8_t data_g_pin;   /**< Green data pin (0xFF = ไม่ใช้) */
    uint8_t data_b_pin;   /**< Blue data pin  (0xFF = ไม่ใช้) */

    /* ----- Control Pins ----- */
    uint8_t clk_pin;      /**< Shift clock pin */
    uint8_t lat_pin;      /**< Latch (STB) pin */
    uint8_t oe_pin;       /**< Output enable pin */

    /* ----- Row Select Pins ----- */
    uint8_t row_a_pin;    /**< Row address A (LSB) */
    uint8_t row_b_pin;    /**< Row address B */
    uint8_t row_c_pin;    /**< Row address C (0xFF = ไม่ใช้) */
    uint8_t row_d_pin;    /**< Row address D (0xFF = ไม่ใช้) */

    /* ----- Panel Geometry ----- */
    uint8_t width;        /**< จำนวนพิกเซลต่อแถว (default 32) */
    uint8_t height;       /**< ความสูงรวม (default 16) */
    uint8_t rows;         /**< จำนวน scan rows (default = height) */

    /* ----- Protocol ----- */
    uint8_t  color_mode;   /**< P10_SINGLE_COLOR / P10_DUAL_COLOR / P10_RGB */
    uint8_t  bit_order;    /**< P10_MSB_FIRST หรือ P10_LSB_FIRST */
    uint16_t refresh_rate; /**< Panel refresh rate (Hz, default 200) */
} P10_Config;

/**
 * @brief Internal pin mapping (for fast ISR access)
 */
typedef struct {
    GPIO_TypeDef* port;  /**< GPIO port */
    uint16_t       pin;  /**< GPIO pin mask */
} P10_PinMap;

/**
 * @brief P10 Panel Instance
 */
typedef struct {
    P10_Config config;              /**< Panel configuration */

    /* ----- Pin maps (internal, for speed) ----- */
    P10_PinMap _data_r;            /**< Internal: R data pin map */
    P10_PinMap _data_g;            /**< Internal: G data pin map */
    P10_PinMap _data_b;            /**< Internal: B data pin map */
    P10_PinMap _clk;               /**< Internal: CLK pin map */
    P10_PinMap _lat;               /**< Internal: LAT pin map */
    P10_PinMap _oe;                /**< Internal: OE pin map */
    P10_PinMap _row_a;             /**< Internal: Row A pin map */
    P10_PinMap _row_b;             /**< Internal: Row B pin map */
    P10_PinMap _row_c;             /**< Internal: Row C pin map */
    P10_PinMap _row_d;             /**< Internal: Row D pin map */

    /* ----- Frame Buffer ----- */
    uint8_t*    framebuffer;       /**< Pointer to frame buffer */
    uint16_t    buffer_size;       /**< Frame buffer size (bytes) */

    /* ----- Scan State (volatile — เปลี่ยนใน ISR) ----- */
    volatile uint8_t current_row;  /**< Current row being scanned (0..rows-1) */

    /* ----- Flags ----- */
    uint8_t initialized;           /**< Initialized flag */
} P10_Instance;

/* ========== Function Prototypes ========== */

/**
 * @brief เริ่มต้น P10 panel
 * @param inst ตัวชี้ไปยัง P10_Instance
 * @param cfg ตัวชี้ไปยัง P10_Config (pin mapping + geometry)
 * @return 1 = สำเร็จ, 0 = ล้มเหลว (null param / bad config)
 *
 * @note ฟังก์ชันนี้จะ:
 *       1. ตั้งค่า GPIO pins ทั้งหมดเป็น output
 *       2. คำนวณ frame buffer ขนาดตาม width × height × planes
 *       3. clear frame buffer
 *       4. เริ่ม timer interrupt scan ที่ refresh_rate × rows Hz
 *
 * @example
 * P10_Instance display;
 * P10_Config cfg = P10_CONFIG_DEFAULT(32, 16);
 * cfg.data_r_pin = PC0;
 * // ... set other pins ...
 * P10_Init(&display, &cfg);
 */
uint8_t P10_Init(P10_Instance* inst, P10_Config* cfg);

/**
 * @brief ตั้งค่าพิกเซลใน frame buffer (On/Off)
 * @param inst ตัวชี้ไปยัง P10_Instance
 * @param x พิกัด X (0 .. width-1)
 * @param y พิกัด Y (0 .. height-1)
 * @param r Red / single color value (0 = off, 1 = on)
 * @param g Green value (0 = off, 1 = on) — ใช้เฉพาะ DUAL/RGB
 * @param b Blue value  (0 = off, 1 = on) — ใช้เฉพาะ RGB
 *
 * @note ค่า r/g/b เป็น 0 หรือ 1 เท่านั้น (ไม่มี grayscale)
 * @note ถ้าใช้ P10_SINGLE_COLOR: แค่ r เป็นตัวกำหนด on/off
 * @note ถ้าใช้ P10_DUAL_COLOR: r=red, g=green
 * @note ถ้าใช้ P10_RGB: r=red, g=green, b=blue
 */
void P10_SetPixel(P10_Instance* inst, uint8_t x, uint8_t y, uint8_t r, uint8_t g, uint8_t b);

/**
 * @brief ลบ frame buffer ทั้งหมด (ปิดทุกพิกเซล)
 * @param inst ตัวชี้ไปยัง P10_Instance
 */
void P10_Clear(P10_Instance* inst);

/**
 * @brief เติม frame buffer ทั้งหมด (เปิดทุกพิกเซลด้วยสีที่กำหนด)
 * @param inst ตัวชี้ไปยัง P10_Instance
 * @param r Red / single color value (0/1)
 * @param g Green value (0/1)
 * @param b Blue value (0/1)
 */
void P10_Fill(P10_Instance* inst, uint8_t r, uint8_t g, uint8_t b);

/**
 * @brief หยุดการทำงานของ P10 panel
 * @param inst ตัวชี้ไปยัง P10_Instance
 *
 * หยุด timer interrupt, ปิด OE, reset state
 */
void P10_Deinit(P10_Instance* inst);

/* ========== Internal Functions (exposed for testing) ========== */

/**
 * @brief ฟังก์ชัน scan handler — เรียกจาก timer ISR
 * @param inst ตัวชี้ไปยัง P10_Instance
 *
 * ขั้นตอน:
 *   1. OE=HIGH (disable output)
 *   2. Set row address (A/B/C/D)
 *   3. Shift out width bits of data (R, G, B ตาม color mode)
 *   4. LAT=HIGH→LOW (latch)
 *   5. OE=LOW (enable output)
 *   6. Increment current_row (wrap at rows)
 */
void P10_ScanHandler(P10_Instance* inst);

#ifdef __cplusplus
}
#endif

#endif /* __P10_H */
