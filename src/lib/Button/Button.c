/**
 * @file Button.c
 * @brief Push Button Library Implementation
 * @version 1.0
 * @date 2026-04-29
 */

#include "Button.h"

/* ========== Public Functions ========== */

/**
 * @brief เริ่มต้น Button instance
 *
 * @param btn - ตัวชี้ไปยัง Button_Instance (NULL → return ทันที)
 * @param pin - GPIO pin number ที่ต่อกับปุ่ม
 * @param active_level - BUTTON_ACTIVE_LOW หรือ BUTTON_ACTIVE_HIGH
 *
 * @note ตั้งค่า GPIO mode ตาม active_level (INPUT_PULLUP สำหรับ LOW, INPUT_PULLDOWN สำหรับ HIGH)
 * @note อ่านสถานะ pin ทันทีหลัง init เพื่อ prime boot-hold state (ถ้ากดค้างตอน boot)
 * @note Default timing: debounce 20ms, long press 800ms, double click 300ms, boot hold 2000ms
 * @note ต้องเรียกก่อนใช้งานฟังก์ชันอื่น ๆ
 */
void Button_Init(Button_Instance* btn, uint8_t pin, Button_ActiveLevel active_level) {
    if (btn == NULL) return;

    btn->pin              = pin;
    btn->active_level     = active_level;
    btn->debounce_ms      = BUTTON_DEFAULT_DEBOUNCE_MS;
    btn->long_press_ms    = BUTTON_DEFAULT_LONG_PRESS_MS;
    btn->double_click_ms  = BUTTON_DEFAULT_DOUBLE_CLICK_MS;

    /* State initialization */
    btn->raw_state        = 0;
    btn->stable_state     = 0;
    btn->is_pressed       = 0;
    btn->long_press_fired = 0;
    btn->last_change_time = 0;
    btn->press_time       = 0;
    btn->last_press_duration_ms = 0;
    btn->last_release_time= 0;
    btn->click_count      = 0;
    btn->last_click_count = 0;
    btn->last_event       = BUTTON_EVENT_NONE;

    btn->init_time        = Get_CurrentMs();
    btn->boot_hold_ms     = BUTTON_DEFAULT_BOOT_HOLD_MS;
    btn->boot_hold_armed  = 0;
    btn->boot_hold_fired  = 0;

    /* Callbacks */
    btn->on_press        = NULL;
    btn->on_release      = NULL;
    btn->on_long_press   = NULL;
    btn->on_double_click = NULL;
    btn->on_multi_click  = NULL;
    btn->on_boot_hold    = NULL;

    btn->initialized = 1;

    /* ตั้งค่า GPIO ตาม active level */
    if (active_level == BUTTON_ACTIVE_LOW) {
        /* กด = LOW: ใช้ internal pull-up */
        pinMode(pin, PIN_MODE_INPUT_PULLUP);
    } else {
        /* กด = HIGH: ใช้ pull-down */
        pinMode(pin, PIN_MODE_INPUT_PULLDOWN);
    }

    /* Prime initial state เพื่อรองรับ boot-hold ตั้งแต่เปิดระบบ */
    {
        uint8_t pin_val = digitalRead(pin);
        uint8_t is_pressed_now = (active_level == BUTTON_ACTIVE_LOW)
                                 ? (pin_val == LOW)
                                 : (pin_val == HIGH);
        btn->raw_state        = is_pressed_now;
        btn->stable_state     = is_pressed_now;
        btn->is_pressed       = is_pressed_now;
        btn->press_time       = btn->init_time;
        btn->last_change_time = btn->init_time;
        btn->boot_hold_armed  = is_pressed_now ? 1 : 0;
    }
}

/**
 * @brief อัพเดตสถานะปุ่ม — ต้องเรียกใน main loop ทุก iteration
 *
 * @param btn - ตัวชี้ไปยัง Button_Instance ที่ init แล้ว (NULL หรือ !initialized → return ทันที)
 *
 * @note ควรเรียกทุก 1-5ms เพื่อ debounce ที่แม่นยำ
 * @note State Machine: อ่าน raw pin → แปลง pressed logic → debounce → ตรวจจับ event → เรียก callback
 * @note Debounce: รอให้ pin stable ติดต่อกันนาน debounce_ms ก่อนยอมรับสถานะใหม่
 * @note Long press fire ครั้งเดียวเมื่อกดค้างนานกว่า long_press_ms
 * @note Boot hold: กดค้างตั้งแต่ init นานกว่า boot_hold_ms → fire BUTTON_EVENT_BOOT_HOLD
 * @note Click finalize: หลังจากปล่อยปุ่ม รอ double_click_ms → สรุป single/double/multi click
 */
void Button_Update(Button_Instance* btn) {
    if (btn == NULL || !btn->initialized) return;

    uint32_t now = Get_CurrentMs();

    /* อ่านสถานะ pin และแปลงเป็น "pressed" logic */
    uint8_t pin_val = digitalRead(btn->pin);
    uint8_t is_raw_pressed = (btn->active_level == BUTTON_ACTIVE_LOW)
                             ? (pin_val == LOW)
                             : (pin_val == HIGH);

    /* ===== Debounce ===== */
    if (is_raw_pressed != btn->raw_state) {
        /* pin เปลี่ยนสถานะ → เริ่มนับ debounce */
        btn->raw_state       = is_raw_pressed;
        btn->last_change_time = now;
    }

    /* ตรวจสอบว่า pin stable แล้วหรือยัง */
    if ((now - btn->last_change_time) < btn->debounce_ms) {
        /* ยังอยู่ใน debounce window → ไม่ทำอะไร */
        goto check_long_press;
    }

    /* pin stable แล้ว ตรวจสอบการเปลี่ยนสถานะ */
    if (is_raw_pressed != btn->stable_state) {
        btn->stable_state = is_raw_pressed;

        if (is_raw_pressed) {
            /* ===== กดปุ่ม (Press) ===== */
            btn->is_pressed       = 1;
            btn->long_press_fired = 0;
            btn->press_time       = now;

            btn->last_event = BUTTON_EVENT_PRESS;
            if (btn->on_press) btn->on_press();

        } else {
            /* ===== ปล่อยปุ่ม (Release) ===== */
            btn->is_pressed       = 0;
            uint32_t prev_release_time = btn->last_release_time;
            btn->last_release_time = now;

            btn->last_press_duration_ms = now - btn->press_time;

            /* นับ click เฉพาะกรณีไม่ใช่ long press */
            if (!btn->long_press_fired) {
                if (btn->click_count == 0 ||
                    (now - prev_release_time) < btn->double_click_ms) {
                    btn->click_count++;
                } else {
                    btn->click_count = 1;
                }
            } else {
                btn->click_count = 0;
            }

            /* เมื่อปล่อยครั้งแรกหลังบูต ถือว่าไม่ใช่การค้างตั้งแต่เปิดระบบแล้ว */
            btn->boot_hold_armed = 0;

            btn->last_event = BUTTON_EVENT_RELEASE;
            if (btn->on_release) btn->on_release();
        }
    }

check_long_press:
    /* ===== Long Press ===== */
    if (btn->is_pressed && !btn->long_press_fired) {
        if ((now - btn->press_time) >= btn->long_press_ms) {
            btn->long_press_fired = 1;
            btn->click_count      = 0;  /* ยกเลิก click count เพราะเป็น long press */
            btn->last_event       = BUTTON_EVENT_LONG_PRESS;
            if (btn->on_long_press) btn->on_long_press();
        }
    }

    /* ===== Boot Hold ===== */
    if (btn->boot_hold_armed && !btn->boot_hold_fired && btn->is_pressed) {
        if ((now - btn->init_time) >= btn->boot_hold_ms) {
            btn->boot_hold_fired = 1;
            btn->last_event      = BUTTON_EVENT_BOOT_HOLD;
            if (btn->on_boot_hold) btn->on_boot_hold();
        }
    }

    /* ===== Click Finalize Timeout ===== */
    if (btn->click_count == 1 && !btn->is_pressed) {
        if ((now - btn->last_release_time) >= btn->double_click_ms) {
            btn->last_click_count = 1;
            btn->click_count = 0;  /* หมดเวลา double click window */
        }
    }

    if (btn->click_count >= 2 && !btn->is_pressed) {
        if ((now - btn->last_release_time) >= btn->double_click_ms) {
            uint8_t final_clicks = btn->click_count;
            btn->last_click_count = final_clicks;
            btn->click_count = 0;

            if (final_clicks == 2) {
                btn->last_event = BUTTON_EVENT_DOUBLE_CLICK;
                if (btn->on_double_click) btn->on_double_click();
            } else {
                btn->last_event = BUTTON_EVENT_MULTI_CLICK;
                if (btn->on_multi_click) btn->on_multi_click(final_clicks);
            }
        }
    }
}

/**
 * @brief ตั้งค่า Callbacks สำหรับ events หลัก
 *
 * @param btn - ตัวชี้ไปยัง Button_Instance (NULL → return ทันที)
 * @param on_press - callback เมื่อกดปุ่ม (NULL = ไม่ใช้)
 * @param on_release - callback เมื่อปล่อยปุ่ม (NULL = ไม่ใช้)
 * @param on_long_press - callback เมื่อกดค้าง (NULL = ไม่ใช้)
 * @param on_double_click - callback เมื่อ double click (NULL = ไม่ใช้)
 *
 * @note เรียกใช้หลัง Button_Init() เท่านั้น
 * @note ถ้าต้องการ on_multi_click หรือ on_boot_hold ให้ใช้ Button_SetAdvancedCallback()
 * @note Callback จะถูกเรียกจากภายใน Button_Update() เท่านั้น
 */
void Button_SetCallback(Button_Instance* btn,
                        void (*on_press)(void),
                        void (*on_release)(void),
                        void (*on_long_press)(void),
                        void (*on_double_click)(void)) {
    if (btn == NULL) return;
    btn->on_press        = on_press;
    btn->on_release      = on_release;
    btn->on_long_press   = on_long_press;
    btn->on_double_click = on_double_click;
}

/**
 * @brief ตั้งค่า Callbacks ขั้นสูงสำหรับ multi-click และ boot-hold
 *
 * @param btn - ตัวชี้ไปยัง Button_Instance (NULL → return ทันที)
 * @param on_multi_click - callback เมื่อกดติดกัน >= 3 ครั้ง (NULL = ไม่ใช้) — ได้รับ click_count เป็นพารามิเตอร์
 * @param on_boot_hold - callback เมื่อกดค้างตั้งแต่เปิดระบบจนถึง boot_hold_ms (NULL = ไม่ใช้)
 *
 * @note เรียกใช้หลัง Button_Init() หรือหลัง Button_SetCallback() ก็ได้
 * @note boot-hold จะ fire เพียงครั้งเดียวหลังจาก init จนกว่าจะมีการปล่อยปุ่มครั้งแรก
 */
void Button_SetAdvancedCallback(Button_Instance* btn,
                                void (*on_multi_click)(uint8_t click_count),
                                void (*on_boot_hold)(void)) {
    if (btn == NULL) return;
    btn->on_multi_click = on_multi_click;
    btn->on_boot_hold   = on_boot_hold;
}

/**
 * @brief ตรวจสอบว่าปุ่มถูกกดอยู่หรือไม่
 *
 * @param btn - ตัวชี้ไปยัง Button_Instance (NULL หรือ !initialized → return false)
 * @return true ถ้ากำลังกดปุ่มอยู่, false ถ้าไม่ได้กดหรือ instance ไม่ถูกต้อง
 *
 * @note คืนค่า based on debounced state (stable_state) ไม่ใช่ raw pin
 * @note ใช้สำหรับ polling ใน main loop
 */
bool Button_IsPressed(Button_Instance* btn) {
    if (btn == NULL || !btn->initialized) return false;
    return btn->is_pressed == 1;
}

/**
 * @brief ดึง event ล่าสุดแล้วล้าง (polling mode)
 *
 * @param btn - ตัวชี้ไปยัง Button_Instance (NULL หรือ !initialized → return BUTTON_EVENT_NONE)
 * @return Button_Event - event ล่าสุด หรือ BUTTON_EVENT_NONE
 *
 * @note หลังเรียกครั้งเดียว event จะถูกล้างเป็น BUTTON_EVENT_NONE ทันที (consume-once)
 * @note ใช้แทน callback mode: เรียกใน loop แล้วตรวจสอบค่าที่ return
 */
Button_Event Button_GetEvent(Button_Instance* btn) {
    if (btn == NULL || !btn->initialized) return BUTTON_EVENT_NONE;
    Button_Event ev = btn->last_event;
    btn->last_event = BUTTON_EVENT_NONE;
    return ev;
}

/**
 * @brief ตั้งค่า debounce time
 *
 * @param btn - ตัวชี้ไปยัง Button_Instance (NULL → return ทันที)
 * @param ms - Debounce time (ms) — ค่า default: 20ms
 *
 * @note ควรตั้งตาม noise characteristic ของปุ่ม ค่าปกติ 10-50ms
 * @note ค่าน้อยไป → false trigger, ค่ามากไป → response ช้า
 */
void Button_SetDebounce(Button_Instance* btn, uint16_t ms) {
    if (btn == NULL) return;
    btn->debounce_ms = ms;
}

/**
 * @brief ตั้งค่า long press threshold
 *
 * @param btn - ตัวชี้ไปยัง Button_Instance (NULL → return ทันที)
 * @param ms - เวลากดค้างขั้นต่ำ (ms) — ค่า default: 800ms
 *
 * @note Long press fire ครั้งเดียว (เมื่อกดค้างเกินเวลาที่กำหนด)
 * @note เมื่อ long press fire แล้ว click_count จะถูกรีเซ็ตเป็น 0
 */
void Button_SetLongPressTime(Button_Instance* btn, uint32_t ms) {
    if (btn == NULL) return;
    btn->long_press_ms = ms;
}

/**
 * @brief ตั้งค่า double click window
 *
 * @param btn - ตัวชี้ไปยัง Button_Instance (NULL → return ทันที)
 * @param ms - ช่วงเวลาระหว่างสองครั้ง (ms) — ค่า default: 300ms
 *
 * @note หน้าต่างเวลาสำหรับ click finalization: กดครั้งที่สองภายในเวลานี้ → double click
 * @note ถ้ากดครั้งที่สามภายในเวลานี้ → multi click (>=3)
 */
void Button_SetDoubleClickTime(Button_Instance* btn, uint16_t ms) {
    if (btn == NULL) return;
    btn->double_click_ms = ms;
}

/**
 * @brief ตั้งค่าเวลา boot-hold threshold
 *
 * @param btn - ตัวชี้ไปยัง Button_Instance (NULL → return ทันที)
 * @param ms - เวลากดค้างตั้งแต่เปิดระบบเพื่อ trigger boot-hold event (ms) — ค่า default: 2000ms
 *
 * @note Boot hold จะ fire เมื่อกดปุ่มค้างตั้งแต่ init โดยไม่ปล่อยเลย
 * @note ใช้สำหรับ factory reset หรือเข้าสู่ bootloader mode
 */
void Button_SetBootHoldTime(Button_Instance* btn, uint32_t ms) {
    if (btn == NULL) return;
    btn->boot_hold_ms = ms;
}

/**
 * @brief อ่านระยะเวลาที่กดค้างอยู่ในขณะนี้ (ms)
 *
 * @param btn - ตัวชี้ไปยัง Button_Instance (NULL หรือ !initialized → return 0)
 * @return ระยะเวลาตั้งแต่กดจนถึงปัจจุบัน (ms), 0 ถ้าไม่ได้กดอยู่
 *
 * @note เวลาจะนับต่อเนื่องตราบที่ยังกดปุ่มอยู่ (real-time)
 * @note ใช้สำหรับวัดระยะเวลากดแบบ real-time เช่น UI feedback
 */
uint32_t Button_GetPressDurationMs(Button_Instance* btn) {
    if (btn == NULL || !btn->initialized) return 0;
    if (!btn->is_pressed) return 0;
    return Get_CurrentMs() - btn->press_time;
}

/**
 * @brief อ่านระยะเวลาการกดครั้งล่าสุดที่สมบูรณ์แล้ว
 *
 * @param btn - ตัวชี้ไปยัง Button_Instance (NULL หรือ !initialized → return 0)
 * @return ระยะเวลาการกดครั้งล่าสุด (ms)
 *
 * @note ค่าถูกบันทึกตอนปล่อยปุ่ม (release) และคงอยู่จนกว่าจะกดครั้งถัดไป
 * @note ไม่ใช่ real-time แต่เป็นค่าของการกดครั้งก่อน
 */
uint32_t Button_GetLastPressDurationMs(Button_Instance* btn) {
    if (btn == NULL || !btn->initialized) return 0;
    return btn->last_press_duration_ms;
}

/**
 * @brief อ่านจำนวนคลิกล่าสุดที่ถูก finalize แล้ว
 *
 * @param btn - ตัวชี้ไปยัง Button_Instance (NULL หรือ !initialized → return 0)
 * @return จำนวนคลิกล่าสุด (0 = ยังไม่มี, 1 = single, 2 = double, >=3 = multi)
 *
 * @note Finalize เกิดขึ้นหลังจากปล่อยปุ่มและพ้น double_click_window แล้ว
 * @note ถ้ามี long press → click_count จะเป็น 0 เสมอ
 */
uint8_t Button_GetLastClickCount(Button_Instance* btn) {
    if (btn == NULL || !btn->initialized) return 0;
    return btn->last_click_count;
}

/**
 * @brief Reset state ของปุ่ม (ล้าง event และ state ทั้งหมด)
 *
 * @param btn - ตัวชี้ไปยัง Button_Instance (NULL → return ทันที)
 *
 * @note รีเซ็ตทุก state ยกเว้น pin configuration, active_level, timing config และ callbacks
 * @note init_time จะถูกเซ็ตเป็นเวลาปัจจุบัน ทำให้ boot-hold เริ่มนับใหม่
 * @note หลัง Reset ปุ่มจะอยู่ในสถานะ idle รอการกดครั้งแรก
 */
void Button_Reset(Button_Instance* btn) {
    if (btn == NULL) return;
    btn->raw_state         = 0;
    btn->stable_state      = 0;
    btn->is_pressed        = 0;
    btn->long_press_fired  = 0;
    btn->last_change_time  = 0;
    btn->press_time        = 0;
    btn->last_press_duration_ms = 0;
    btn->last_release_time = 0;
    btn->click_count       = 0;
    btn->last_click_count  = 0;
    btn->last_event        = BUTTON_EVENT_NONE;
    btn->init_time         = Get_CurrentMs();
    btn->boot_hold_armed   = 0;
    btn->boot_hold_fired   = 0;
}

/**
 * @brief แปลง Button_Event เป็น string สำหรับ debug
 *
 * @param event - Button_Event ที่ต้องการแปลง
 * @return const char* - ชื่อ event ในรูป string (เช่น "PRESS", "LONG_PRESS")
 *
 * @note ถ้า event ไม่รู้จัก → return "UNKNOWN"
 * @note ใช้กับ printf() หรือ UART debug output
 */
const char* Button_EventStr(Button_Event event) {
    switch (event) {
        case BUTTON_EVENT_NONE:         return "NONE";
        case BUTTON_EVENT_PRESS:        return "PRESS";
        case BUTTON_EVENT_RELEASE:      return "RELEASE";
        case BUTTON_EVENT_LONG_PRESS:   return "LONG_PRESS";
        case BUTTON_EVENT_DOUBLE_CLICK: return "DOUBLE_CLICK";
        case BUTTON_EVENT_MULTI_CLICK:  return "MULTI_CLICK";
        case BUTTON_EVENT_BOOT_HOLD:    return "BOOT_HOLD";
        default:                        return "UNKNOWN";
    }
}
