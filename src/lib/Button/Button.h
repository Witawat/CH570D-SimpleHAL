/**
 * @file Button.h
 * @brief Push Button Library สำหรับ CH57x
 * @version 1.0
 * @date 2026-04-29
 *
 * @details
 * Library สำหรับจัดการปุ่มกด (Push Button) แบบครบวงจร
 * รองรับ debounce, single press, long press, double click
 * พร้อม callback system
 *
 * **คุณสมบัติ:**
 * - Debounce อัตโนมัติ (ป้องกัน noise จากการกด)
 * - Single Press detection
 * - Release detection
 * - Long Press detection (กดค้าง)
 * - Double Click detection
 * - Callback system สำหรับแต่ละ event
 * - รองรับทั้ง Active LOW และ Active HIGH
 * - Multi-button support (สูงสุด 8 ปุ่ม)
 * - ใช้แบบ polling (เรียก Button_Update() ใน loop)
 *
 * **หลักการ Active LOW vs Active HIGH:**
 * - Active LOW (แนะนำ): ปุ่มต่อระหว่าง pin และ GND, ใช้ pull-up ใน CH57x
 * - Active HIGH: ปุ่มต่อระหว่าง pin และ VCC, ใช้ pull-down
 *
 * **Hardware Connection (Active LOW - แนะนำ):**
 * ```
 *   3.3V
 *    |
 *   [10kΩ] หรือใช้ internal pull-up
 *    |
 *   GPIO -----> Button -----> GND
 *   (input)
 *
 *   เมื่อกด: GPIO = LOW
 *   เมื่อปล่อย: GPIO = HIGH
 * ```
 *
 * @example
 * #include "SimpleHAL.h"
 * #include "Button.h"
 *
 * Button_Instance btn;
 *
 * void on_press(void) {
 *     printf("กดปุ่ม!\r\n");
 * }
 *
 * void on_long_press(void) {
 *     printf("กดค้าง!\r\n");
 * }
 *
 * int main(void) {
 *     SystemCoreClockUpdate();
 *     Timer_Init();
 *
 *     Button_Init(&btn, PC4, BUTTON_ACTIVE_LOW);
 *     Button_SetCallback(&btn, on_press, NULL, on_long_press, NULL);
 *
 *     while (1) {
 *         Button_Update(&btn);  // ต้องเรียกใน loop เสมอ
 *         Delay_Ms(1);
 *     }
 * }
 *
 * @author CH57x Library Team
 * @copyright MIT License
 */

#ifndef __BUTTON_H
#define __BUTTON_H

#ifdef __cplusplus
extern "C" {
#endif

#include "../../SimpleHAL/SimpleHAL.h"
#include <stdint.h>
#include <stdbool.h>

/* ========== Configuration ========== */

/**
 * @brief จำนวน Button instances สูงสุด
 */
#ifndef BUTTON_MAX_INSTANCES
#define BUTTON_MAX_INSTANCES    8
#endif

/**
 * @brief เวลา debounce default (ms)
 * @note ปรับได้ต่อ instance ด้วย Button_SetDebounce()
 */
#define BUTTON_DEFAULT_DEBOUNCE_MS      20

/**
 * @brief เวลา long press default (ms)
 * @note ปรับได้ต่อ instance ด้วย Button_SetLongPressTime()
 */
#define BUTTON_DEFAULT_LONG_PRESS_MS    800

/**
 * @brief เวลา double click window (ms)
 * @note กดสองครั้งภายในเวลานี้ถือเป็น double click
 */
#define BUTTON_DEFAULT_DOUBLE_CLICK_MS  300

/**
 * @brief เวลา boot-hold default (ms)
 * @note กดค้างตั้งแต่เปิดระบบถึงเวลานี้จะ fire BUTTON_EVENT_BOOT_HOLD
 */
#define BUTTON_DEFAULT_BOOT_HOLD_MS     2000

/* ========== Type Definitions ========== */

/**
 * @brief ประเภทการ active ของปุ่ม
 */
typedef enum {
    BUTTON_ACTIVE_LOW  = 0, /**< กด = LOW (ใช้ pull-up) — แนะนำ */
    BUTTON_ACTIVE_HIGH = 1  /**< กด = HIGH (ใช้ pull-down) */
} Button_ActiveLevel;

/**
 * @brief Event ของปุ่ม
 */
typedef enum {
    BUTTON_EVENT_NONE         = 0,  /**< ไม่มี event */
    BUTTON_EVENT_PRESS        = 1,  /**< กดปุ่ม (leading edge) */
    BUTTON_EVENT_RELEASE      = 2,  /**< ปล่อยปุ่ม (trailing edge) */
    BUTTON_EVENT_LONG_PRESS   = 3,  /**< กดค้างเกิน long_press_ms */
    BUTTON_EVENT_DOUBLE_CLICK = 4,  /**< กดสองครั้งรวดเร็ว */
    BUTTON_EVENT_MULTI_CLICK  = 5,  /**< กดหลายครั้งติดกัน (>= 3) */
    BUTTON_EVENT_BOOT_HOLD    = 6   /**< กดค้างตั้งแต่เปิดระบบ */
} Button_Event;

/**
 * @brief โครงสร้างข้อมูล Button instance
 *
 * @note ประกาศเป็น global หรือ static ไม่ใช่ local variable
 */
typedef struct {
    /* Pin Configuration */
    uint8_t              pin;               /**< GPIO pin */
    Button_ActiveLevel   active_level;      /**< Active LOW หรือ HIGH */

    /* Timing Configuration */
    uint16_t             debounce_ms;       /**< Debounce time (ms) */
    uint32_t             long_press_ms;     /**< Long press threshold (ms) */
    uint16_t             double_click_ms;   /**< Double click window (ms) */

    /* State (internal) */
    uint8_t              raw_state;         /**< raw pin reading */
    uint8_t              stable_state;      /**< debounced state */
    uint8_t              is_pressed;        /**< pressed = 1 */
    uint8_t              long_press_fired;  /**< long press callback ถูก fire แล้ว */
    uint32_t             last_change_time;  /**< เวลา state เปลี่ยน (debounce) */
    uint32_t             press_time;        /**< เวลาที่เริ่มกด */
    uint32_t             last_press_duration_ms; /**< ระยะเวลาการกดครั้งล่าสุด */
    uint32_t             last_release_time; /**< เวลาปล่อยครั้งล่าสุด */
    uint8_t              click_count;       /**< นับ click สำหรับ double click */
    uint8_t              last_click_count;  /**< จำนวนคลิกล่าสุดที่ finalize แล้ว */
    Button_Event         last_event;        /**< event ล่าสุด */

    /* Boot-hold configuration/state */
    uint32_t             init_time;         /**< เวลา init ปุ่ม */
    uint32_t             boot_hold_ms;      /**< เวลาค้างตั้งแต่เปิดระบบเพื่อ fire boot-hold */
    uint8_t              boot_hold_armed;   /**< armed ถ้าปุ่มถูกกดตั้งแต่ init */
    uint8_t              boot_hold_fired;   /**< boot-hold event ถูก fire แล้ว */

    /* Callbacks */
    void (*on_press)(void);         /**< Callback เมื่อกดปุ่ม */
    void (*on_release)(void);       /**< Callback เมื่อปล่อยปุ่ม */
    void (*on_long_press)(void);    /**< Callback เมื่อกดค้าง */
    void (*on_double_click)(void);  /**< Callback เมื่อ double click */
    void (*on_multi_click)(uint8_t click_count); /**< Callback เมื่อ multi click (>=3) */
    void (*on_boot_hold)(void);     /**< Callback เมื่อกดค้างตั้งแต่เปิดระบบ */

    uint8_t              initialized;   /**< flag init */
} Button_Instance;

/* ========== Function Prototypes ========== */

/**
 * @brief เริ่มต้น Button instance
 *
 * @param btn          ตัวชี้ไปยัง Button_Instance
 * @param pin          GPIO pin ที่ต่อกับปุ่ม
 * @param active_level BUTTON_ACTIVE_LOW หรือ BUTTON_ACTIVE_HIGH
 *
 * @note ใช้ BUTTON_ACTIVE_LOW กับ internal pull-up (แนะนำสำหรับทั่วไป)
 *
 * @example
 * Button_Instance btn;
 * Button_Init(&btn, PC4, BUTTON_ACTIVE_LOW);
 */
void Button_Init(Button_Instance* btn, uint8_t pin, Button_ActiveLevel active_level);

/**
 * @brief อัพเดตสถานะปุ่ม — ต้องเรียกใน main loop ทุก iteration
 *
 * @param btn  ตัวชี้ไปยัง Button_Instance
 *
 * @note ควรเรียกทุก 1-5ms เพื่อ debounce ที่แม่นยำ
 * @note Callback จะถูกเรียกจากฟังก์ชันนี้
 *
 * @example
 * while (1) {
 *     Button_Update(&btn);
 *     Delay_Ms(1);  // ทุก 1ms
 * }
 */
void Button_Update(Button_Instance* btn);

/**
 * @brief ตั้งค่า Callback สำหรับ events ต่างๆ
 *
 * @param btn              ตัวชี้ไปยัง Button_Instance
 * @param on_press         เรียกเมื่อกดปุ่ม (NULL = ไม่ใช้)
 * @param on_release       เรียกเมื่อปล่อยปุ่ม (NULL = ไม่ใช้)
 * @param on_long_press    เรียกเมื่อกดค้าง (NULL = ไม่ใช้)
 * @param on_double_click  เรียกเมื่อ double click (NULL = ไม่ใช้)
 *
 * @example
 * Button_SetCallback(&btn, my_press, my_release, my_long, my_double);
 */
void Button_SetCallback(Button_Instance* btn,
                        void (*on_press)(void),
                        void (*on_release)(void),
                        void (*on_long_press)(void),
                        void (*on_double_click)(void));

/**
 * @brief ตั้งค่า callback ขั้นสูง
 *
 * @param btn             ตัวชี้ไปยัง Button_Instance
 * @param on_multi_click  เรียกเมื่อกดติดกันหลายครั้ง (>=3)
 * @param on_boot_hold    เรียกเมื่อกดค้างตั้งแต่เปิดระบบ
 */
void Button_SetAdvancedCallback(Button_Instance* btn,
                                void (*on_multi_click)(uint8_t click_count),
                                void (*on_boot_hold)(void));

/**
 * @brief ตรวจสอบว่าปุ่มถูกกดอยู่หรือไม่ (polling)
 *
 * @param btn  ตัวชี้ไปยัง Button_Instance
 * @return true ถ้ากดอยู่
 *
 * @example
 * if (Button_IsPressed(&btn)) {
 *     printf("ปุ่มถูกกดอยู่\r\n");
 * }
 */
bool Button_IsPressed(Button_Instance* btn);

/**
 * @brief ตรวจสอบและล้าง event ล่าสุด (polling mode ไม่ใช้ callback)
 *
 * @param btn  ตัวชี้ไปยัง Button_Instance
 * @return Button_Event event ล่าสุด หรือ BUTTON_EVENT_NONE
 *
 * @note หลังเรียกครั้งเดียว event จะถูกล้างเป็น NONE
 *
 * @example
 * // ใช้แบบ polling แทน callback
 * Button_Event ev = Button_GetEvent(&btn);
 * if (ev == BUTTON_EVENT_PRESS)        { handle_press(); }
 * if (ev == BUTTON_EVENT_LONG_PRESS)   { handle_long();  }
 * if (ev == BUTTON_EVENT_DOUBLE_CLICK) { handle_double();}
 */
Button_Event Button_GetEvent(Button_Instance* btn);

/**
 * @brief ตั้งค่า debounce time
 *
 * @param btn  ตัวชี้ไปยัง Button_Instance
 * @param ms   Debounce time (ms) — ค่า default: 20ms
 */
void Button_SetDebounce(Button_Instance* btn, uint16_t ms);

/**
 * @brief ตั้งค่า long press threshold
 *
 * @param btn  ตัวชี้ไปยัง Button_Instance
 * @param ms   เวลากดค้างขั้นต่ำ (ms) — ค่า default: 800ms
 */
void Button_SetLongPressTime(Button_Instance* btn, uint32_t ms);

/**
 * @brief ตั้งค่า double click window
 *
 * @param btn  ตัวชี้ไปยัง Button_Instance
 * @param ms   ช่วงเวลาระหว่างสองครั้ง (ms) — ค่า default: 300ms
 */
void Button_SetDoubleClickTime(Button_Instance* btn, uint16_t ms);

/**
 * @brief ตั้งค่าเวลา boot-hold threshold
 *
 * @param btn  ตัวชี้ไปยัง Button_Instance
 * @param ms   เวลากดค้างตั้งแต่เปิดระบบเพื่อ trigger boot-hold event (ms)
 */
void Button_SetBootHoldTime(Button_Instance* btn, uint32_t ms);

/**
 * @brief อ่านระยะเวลาที่กดค้างอยู่ในขณะนี้ (ms)
 *
 * @param btn  ตัวชี้ไปยัง Button_Instance
 * @return ระยะเวลา (ms), 0 ถ้าไม่ได้กดอยู่
 */
uint32_t Button_GetPressDurationMs(Button_Instance* btn);

/**
 * @brief อ่านระยะเวลาการกดครั้งล่าสุด (ms)
 *
 * @param btn  ตัวชี้ไปยัง Button_Instance
 * @return ระยะเวลาการกดครั้งล่าสุด (ms)
 */
uint32_t Button_GetLastPressDurationMs(Button_Instance* btn);

/**
 * @brief อ่านจำนวนคลิกล่าสุดที่ถูก finalize แล้ว
 *
 * @param btn  ตัวชี้ไปยัง Button_Instance
 * @return จำนวนคลิกล่าสุด (0 = ยังไม่มี)
 */
uint8_t Button_GetLastClickCount(Button_Instance* btn);

/**
 * @brief Reset state ของปุ่ม (ล้าง event และ state ทั้งหมด)
 *
 * @param btn  ตัวชี้ไปยัง Button_Instance
 */
void Button_Reset(Button_Instance* btn);

/**
 * @brief แปลง Button_Event เป็น string สำหรับ debug
 *
 * @param event  event ที่ต้องการแปลง
 * @return string อธิบาย event
 *
 * @example
 * printf("Event: %s\r\n", Button_EventStr(ev));
 */
const char* Button_EventStr(Button_Event event);

#ifdef __cplusplus
}
#endif

#endif /* __BUTTON_H */
