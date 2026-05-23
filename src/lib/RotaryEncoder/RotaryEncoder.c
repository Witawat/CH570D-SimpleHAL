/**
 * @file RotaryEncoder.c
 * @brief Rotary Encoder KY-040 Library Implementation
 * @version 1.0
 * @date 2025-12-22
 */

#include "RotaryEncoder.h"
#include <string.h>

/* ========== Private Variables ========== */

// Global encoder instances for interrupt handling
static RotaryEncoder* g_encoders[8] = {NULL};  // Support up to 8 encoders
static uint8_t g_encoder_count = 0;

/* ========== Quadrature Decoding State Machine ========== */

// Gray code transition table for quadrature decoding
// [previous_state][current_state] = direction
// 0 = no change, 1 = CW, -1 = CCW
static const int8_t quadrature_table[4][4] = {
    // Current state: 00  01  10  11
    {  0, -1,  1,  0 },  // Previous: 00
    {  1,  0,  0, -1 },  // Previous: 01
    { -1,  0,  0,  1 },  // Previous: 10
    {  0,  1, -1,  0 }   // Previous: 11
};

/* ========== Private Function Prototypes ========== */

static uint8_t Rotary_ReadState(RotaryEncoder* encoder);
static void Rotary_ProcessRotation(RotaryEncoder* encoder);
static RotaryEncoder* Rotary_FindEncoder(uint8_t pin);

/* ========== Initialization Functions ========== */

/**
 * @brief เริ่มต้น Rotary Encoder
 *
 * @param encoder - instance ที่ผ่านการ memset(0) (NULL → undefined behavior)
 * @param pin_clk - CLK pin (A) — ต้องรองรับ EXTI สำหรับ interrupt
 * @param pin_dt  - DT pin (B) — ต้องรองรับ EXTI สำหรับ interrupt
 * @param pin_sw  - SW pin (Button) — pin ใดก็ได้, 0 = ไม่ใช้
 *
 * @note ตั้งทุก pin เป็น INPUT_PULLUP
 *       attachInterrupt() สำหรับ CLK + DT (CHANGE edge)
 *       อ่าน initial state แล้วเก็บไว้ที่ last_state
 *       ลงทะเบียน encoder ใน g_encoders[] (สูงสุด 8 instances)
 *       ใช้ quadrature table สำหรับถอดรหัสทิศทาง
 */
void Rotary_Init(RotaryEncoder* encoder, uint8_t pin_clk, uint8_t pin_dt, uint8_t pin_sw) {
    // Clear structure
    memset(encoder, 0, sizeof(RotaryEncoder));
    
    // Store pin configuration
    encoder->pin_clk = pin_clk;
    encoder->pin_dt = pin_dt;
    encoder->pin_sw = pin_sw;
    
    // Initialize position
    encoder->position = 0;
    encoder->last_position = 0;
    encoder->use_limits = false;
    
    // Initialize direction
    encoder->direction = ROTARY_DIR_NONE;
    
    // Initialize debouncing
    encoder->debounce_ms = ROTARY_DEFAULT_DEBOUNCE_MS;
    encoder->last_change_time = 0;
    
    // Initialize acceleration
    encoder->acceleration_enabled = false;
    encoder->acceleration_factor = 1;
    encoder->last_rotation_time = 0;
    
    // Initialize button
    encoder->button_pressed = false;
    encoder->button_last_state = false;
    encoder->button_press_time = 0;
    encoder->button_release_time = 0;
    encoder->click_count = 0;
    
    // Configure pins
    pinMode(pin_clk, PIN_MODE_INPUT_PULLUP);
    pinMode(pin_dt, PIN_MODE_INPUT_PULLUP);
    pinMode(pin_sw, PIN_MODE_INPUT_PULLUP);
    
    // Read initial state
    Delay_Ms(10);  // Wait for pins to stabilize
    encoder->last_state = Rotary_ReadState(encoder);
    
    // Setup interrupts for CLK and DT
    attachInterrupt(pin_clk, (void (*)(void))Rotary_CLK_ISR, CHANGE);
    attachInterrupt(pin_dt, (void (*)(void))Rotary_DT_ISR, CHANGE);
    
    // Register encoder for interrupt handling
    if (g_encoder_count < 8) {
        g_encoders[g_encoder_count++] = encoder;
    }
}

/**
 * @brief รีเซ็ต encoder state
 *
 * @param encoder - instance ที่ผ่านการ init แล้ว
 *
 * @note รีเซ็ต position = 0, direction = NONE
 *       ล้าง acceleration_factor, click_count
 *       ไม่ได้คืนค่า — ใช้ Rotary_SetPosition() ถ้าต้องการ position อื่น
 */
void Rotary_Reset(RotaryEncoder* encoder) {
    encoder->position = 0;
    encoder->last_position = 0;
    encoder->direction = ROTARY_DIR_NONE;
    encoder->acceleration_factor = 1;
    encoder->last_rotation_time = 0;
    encoder->button_pressed = false;
    encoder->button_last_state = false;
    encoder->click_count = 0;
}

/* ========== Position Control Functions ========== */

/**
 * @brief อ่านตำแหน่งปัจจุบัน
 *
 * @param encoder - instance ที่ผ่านการ init แล้ว
 * @return ตำแหน่งปัจจุบัน (signed 32-bit), เคลื่อนที่ละ 1 step ปกติ
 *
 * @note อ่านจาก encoder->position โดยตรง (volatile)
 *       ใน ISR อาจเปลี่ยนค่าได้ — ควรอ่านผ่านฟังก์ชันนี้
 *       ถ้า acceleration เปิด step อาจ > 1
 */
int32_t Rotary_GetPosition(RotaryEncoder* encoder) {
    return encoder->position;
}

/**
 * @brief ตั้งค่าตำแหน่ง
 *
 * @param encoder  - instance ที่ผ่านการ init แล้ว
 * @param position - ตำแหน่งที่ต้องการ set
 *
 * @note ตั้งทั้ง position และ last_position ให้เท่ากัน
 *       เพื่อให้ HasChanged() return false หลัง set
 */
void Rotary_SetPosition(RotaryEncoder* encoder, int32_t position) {
    encoder->position = position;
    encoder->last_position = position;
}

/**
 * @brief อ่านทิศทางการหมุนล่าสุด
 *
 * @param encoder - instance ที่ผ่านการ init แล้ว
 * @return ROTARY_DIR_CW, ROTARY_DIR_CCW, หรือ ROTARY_DIR_NONE
 *
 * @note ค่าจะถูกรีเซ็ตเป็น NONE หลัง Rotary_Reset()
 *       direction เปลี่ยนทุกครั้งที่หมุน 1 step
 */
RotaryDirection Rotary_GetDirection(RotaryEncoder* encoder) {
    return encoder->direction;
}

/**
 * @brief ตรวจสอบว่ามีการหมุนหรือไม่
 *
 * @param encoder - instance ที่ผ่านการ init แล้ว
 * @return true ถ้าตำแหน่งเปลี่ยนจากครั้งที่แล้ว
 *
 * @note ใช้ last_position เพื่อตรวจจับ edge
 *       หลัง return true จะ sync last_position = position
 *       ควรเรียกก่อน Rotary_GetPosition() เพื่อเช็คว่าค่าเปลี่ยน
 */
bool Rotary_HasChanged(RotaryEncoder* encoder) {
    if (encoder->position != encoder->last_position) {
        encoder->last_position = encoder->position;
        return true;
    }
    return false;
}

/**
 * @brief ตั้งค่าขอบเขตตำแหน่ง (min/max)
 *
 * @param encoder - instance ที่ผ่านการ init แล้ว
 * @param min     - ตำแหน่งต่ำสุด
 * @param max     - ตำแหน่งสูงสุด
 *
 * @note เปิด use_limits = true
 *       clamp ตำแหน่งปัจจุบันอยู่ในช่วง [min, max]
 *       ถ้า position < min → position = min
 *       ถ้า position > max → position = max
 */
void Rotary_SetLimits(RotaryEncoder* encoder, int32_t min, int32_t max) {
    encoder->min_position = min;
    encoder->max_position = max;
    encoder->use_limits = true;
    
    // Clamp current position
    if (encoder->position < min) {
        encoder->position = min;
    } else if (encoder->position > max) {
        encoder->position = max;
    }
}

/**
 * @brief ยกเลิกการจำกัดตำแหน่ง
 *
 * @param encoder - instance ที่ผ่านการ init แล้ว
 *
 * @note set use_limits = false
 *       ไม่มีการ clamp ตำแหน่งอีกต่อไป
 *       ไม่กระทบค่า position ปัจจุบัน
 */
void Rotary_ClearLimits(RotaryEncoder* encoder) {
    encoder->use_limits = false;
}

/* ========== Button Control Functions ========== */

/**
 * @brief ตรวจสอบว่าปุ่มถูกกดอยู่หรือไม่
 *
 * @param encoder - instance ที่ผ่านการ init แล้ว
 * @return true ถ้าปุ่มถูกกด (SW pin == LOW), false ถ้าปล่อย
 *
 * @note SW pin Active LOW — อ่าน digitalRead โดยตรง
 *       ไม่มี debounce — ใช้ Rotary_UpdateButton() สำหรับ debounce
 *       ถ้า pin_sw == 0 (ไม่ได้ใช้งาน) จะอ่าน pin 0 ซึ่ง undefined
 */
bool Rotary_IsButtonPressed(RotaryEncoder* encoder) {
    return (digitalRead(encoder->pin_sw) == LOW);  // Active LOW
}

/**
 * @brief รอจนกว่าปุ่มจะถูกกดแล้วปล่อย (blocking)
 *
 * @param encoder - instance ที่ผ่านการ init แล้ว
 *
 * @note blocking — ไม่ควรเรียกจาก ISR
 *       รอกด → debounce 50ms → รอปล่อย → debounce 50ms
 *       ใช้ polling (Delay_Ms(10)) — ไม่เหมาะกับระบบ real-time
 */
void Rotary_WaitForButton(RotaryEncoder* encoder) {
    // Wait for button press
    while (!Rotary_IsButtonPressed(encoder)) {
        Delay_Ms(10);
    }
    
    // Debounce
    Delay_Ms(50);
    
    // Wait for button release
    while (Rotary_IsButtonPressed(encoder)) {
        Delay_Ms(10);
    }
    
    // Debounce
    Delay_Ms(50);
}

/**
 * @brief อัปเดต button state (เรียกใน main loop)
 *
 * @param encoder - instance ที่ผ่านการ init แล้ว
 *
 * @note ต้องเรียกฟังก์ชันนี้ใน main loop ทุก 10-50ms
 *       ตรวจจับ press/release/long press/double click
 *       long press = กดค้าง ≥ ROTARY_LONG_PRESS_MS
 *       double click = กด 2 ครั้งใน ROTARY_DOUBLE_CLICK_MS
 *       เรียก callback on_button_press/release/long_press/double_click ถ้ามี
 */
void Rotary_UpdateButton(RotaryEncoder* encoder) {
    bool current_state = Rotary_IsButtonPressed(encoder);
    uint32_t current_time = Get_CurrentMs();
    
    // Detect button press
    if (current_state && !encoder->button_last_state) {
        encoder->button_pressed = true;
        encoder->button_press_time = current_time;
        encoder->click_count++;
        
        // Call press callback
        if (encoder->on_button_press) {
            encoder->on_button_press();
        }
    }
    // Detect button release
    else if (!current_state && encoder->button_last_state) {
        encoder->button_pressed = false;
        encoder->button_release_time = current_time;
        
        uint32_t press_duration = current_time - encoder->button_press_time;
        
        // Check for long press
        if (press_duration >= ROTARY_LONG_PRESS_MS) {
            if (encoder->on_button_long_press) {
                encoder->on_button_long_press();
            }
            encoder->click_count = 0;  // Reset click count after long press
        }
        
        // Call release callback
        if (encoder->on_button_release) {
            encoder->on_button_release();
        }
    }
    
    // Check for double click
    if (encoder->click_count >= 2) {
        uint32_t time_since_first_click = current_time - encoder->button_release_time;
        
        if (time_since_first_click <= ROTARY_DOUBLE_CLICK_MS) {
            if (encoder->on_button_double_click) {
                encoder->on_button_double_click();
            }
            encoder->click_count = 0;
        }
    }
    
    // Reset click count after timeout
    if (encoder->click_count > 0) {
        uint32_t time_since_last_release = current_time - encoder->button_release_time;
        if (time_since_last_release > ROTARY_DOUBLE_CLICK_MS) {
            encoder->click_count = 0;
        }
    }
    
    encoder->button_last_state = current_state;
}

/* ========== Advanced Settings Functions ========== */

/**
 * @brief ตั้งค่าเวลา debounce
 *
 * @param encoder    - instance ที่ผ่านการ init แล้ว
 * @param debounce_ms - เวลา debounce (ms), ค่าเริ่มต้น 5ms
 *
 * @note กระทบ timing ใน Rotary_ProcessRotation()
 *       เพิ่มถ้า encoder มี noise, ลดถ้าต้องการ response ไว
 */
void Rotary_SetDebounceTime(RotaryEncoder* encoder, uint16_t debounce_ms) {
    encoder->debounce_ms = debounce_ms;
}

/**
 * @brief เปิด/ปิด acceleration mode
 *
 * @param encoder - instance ที่ผ่านการ init แล้ว
 * @param enabled - true = เปิด, false = ปิด
 *
 * @note acceleration: หมุนเร็ว → step เพิ่ม (2x, 4x, 8x)
 *       คำนวณ RPM จาก time_diff (20 pulses/รอบของ KY-040)
 *       threshold 60 RPM → 2x, 120 RPM → 4x, 200 RPM → 8x
 *       ปิด → acceleration_factor กลับเป็น 1
 */
void Rotary_SetAcceleration(RotaryEncoder* encoder, bool enabled) {
    encoder->acceleration_enabled = enabled;
    if (!enabled) {
        encoder->acceleration_factor = 1;
    }
}

/* ========== Callback Functions ========== */

/**
 * @brief ตั้งค่า callback เมื่อหมุน encoder
 *
 * @param encoder  - instance ที่ผ่านการ init แล้ว
 * @param callback - ฟังก์ชัน (position, direction), NULL = ยกเลิก
 *
 * @note เรียกจาก Rotary_ProcessRotation() ทุกครั้งที่หมุน 1 step
 *       callback ถูกเรียกใน context ของฟังก์ชันที่เรียก ProcessRotation
 *       (อาจเป็น ISR หรือ main loop)
 */
void Rotary_OnRotate(RotaryEncoder* encoder, void (*callback)(int32_t, RotaryDirection)) {
    encoder->on_rotate = callback;
}

/**
 * @brief ตั้งค่า callback เมื่อกดปุ่ม
 *
 * @param encoder  - instance ที่ผ่านการ init แล้ว
 * @param callback - ฟังก์ชัน void(void), NULL = ยกเลิก
 *
 * @note เรียกจาก Rotary_UpdateButton() เมื่อ detect rising edge (LOW→HIGH)
 */
void Rotary_OnButtonPress(RotaryEncoder* encoder, void (*callback)(void)) {
    encoder->on_button_press = callback;
}

/**
 * @brief ตั้งค่า callback เมื่อปล่อยปุ่ม
 *
 * @param encoder  - instance ที่ผ่านการ init แล้ว
 * @param callback - ฟังก์ชัน void(void), NULL = ยกเลิก
 *
 * @note เรียกจาก Rotary_UpdateButton() เมื่อ detect falling edge (HIGH→LOW)
 */
void Rotary_OnButtonRelease(RotaryEncoder* encoder, void (*callback)(void)) {
    encoder->on_button_release = callback;
}

/**
 * @brief ตั้งค่า callback เมื่อกดปุ่มค้าง
 *
 * @param encoder  - instance ที่ผ่านการ init แล้ว
 * @param callback - ฟังก์ชัน void(void), NULL = ยกเลิก
 *
 * @note เรียกจาก Rotary_UpdateButton() เมื่อ press_duration ≥ ROTARY_LONG_PRESS_MS
 *       long press → click_count ถูกรีเซ็ต (ไม่นับ double click ต่อ)
 */
void Rotary_OnButtonLongPress(RotaryEncoder* encoder, void (*callback)(void)) {
    encoder->on_button_long_press = callback;
}

/**
 * @brief ตั้งค่า callback เมื่อดับเบิลคลิก
 *
 * @param encoder  - instance ที่ผ่านการ init แล้ว
 * @param callback - ฟังก์ชัน void(void), NULL = ยกเลิก
 *
 * @note เรียกจาก Rotary_UpdateButton() เมื่อ click_count ≥ 2
 *       ภายใน ROTARY_DOUBLE_CLICK_MS
 *       click_count timeout ถ้าเกิน ROTARY_DOUBLE_CLICK_MS
 */
void Rotary_OnButtonDoubleClick(RotaryEncoder* encoder, void (*callback)(void)) {
    encoder->on_button_double_click = callback;
}

/* ========== Core Processing Functions ========== */

/**
 * @brief อัปเดต encoder state (เรียกใน main loop)
 *
 * @param encoder - instance ที่ผ่านการ init แล้ว
 *
 * @note เรียก Rotary_ProcessRotation() เพื่ออ่าน state encoder
 *       ใช้สำหรับ polling mode (ไม่ใช้ interrupt)
 *       กรณีใช้ interrupt — เรียกอัตโนมัติจาก CLK_ISR / DT_ISR
 */
void Rotary_Update(RotaryEncoder* encoder) {
    Rotary_ProcessRotation(encoder);
}

/**
 * @brief Interrupt handler สำหรับ CLK pin
 *
 * @param encoder - instance ที่ผ่านการ init แล้ว
 *
 * @note ถูก attachInterrupt() ตอน Rotary_Init()
 *       เรียก Rotary_ProcessRotation() — ใช้ quadrature decoding
 *       ทำงานใน ISR context — ห้ามใช้ blocking operation
 */
void Rotary_CLK_ISR(RotaryEncoder* encoder) {
    Rotary_ProcessRotation(encoder);
}

/**
 * @brief Interrupt handler สำหรับ DT pin
 *
 * @param encoder - instance ที่ผ่านการ init แล้ว
 *
 * @note ถูก attachInterrupt() ตอน Rotary_Init()
 *       เรียก Rotary_ProcessRotation() — ใช้ quadrature decoding
 *       ทำงานใน ISR context — ห้ามใช้ blocking operation
 */
void Rotary_DT_ISR(RotaryEncoder* encoder) {
    Rotary_ProcessRotation(encoder);
}

/* ========== Private Functions ========== */

/**
 * @brief อ่าน state ของ encoder (2-bit)
 *
 * @param encoder - instance ที่ผ่านการ init แล้ว
 * @return State (0-3): bit1 = CLK, bit0 = DT
 *
 * @note digitalRead CLK และ DT pin
 *       return (clk << 1) | dt
 *       ใช้เป็น index สำหรับ quadrature lookup table [previous][current]
 */
static uint8_t Rotary_ReadState(RotaryEncoder* encoder) {
    uint8_t clk = digitalRead(encoder->pin_clk);
    uint8_t dt = digitalRead(encoder->pin_dt);
    return (clk << 1) | dt;
}

/**
 * @brief ประมวลผลการหมุน encoder (quadrature decoding)
 *
 * @param encoder - instance ที่ผ่านการ init แล้ว
 *
 * @note ใช้ quadrature_table[last_state][current_state] → ±1
 *       debounce เช็ค time_diff ≥ debounce_ms ก่อนอ่าน
 *       acceleration: คำนวณ RPM (20 pulses/รอบสำหรับ KY-040)
 *       clamp position ถ้า use_limits
 *       เรียก on_rotate callback ถ้ามี
 */
static void Rotary_ProcessRotation(RotaryEncoder* encoder) {
    uint32_t current_time = Get_CurrentMs();
    
    // Debouncing
    if ((current_time - encoder->last_change_time) < encoder->debounce_ms) {
        return;
    }
    
    // Read current state
    uint8_t current_state = Rotary_ReadState(encoder);
    
    // Check if state changed
    if (current_state == encoder->last_state) {
        return;
    }
    
    // Get direction from quadrature table
    int8_t direction = quadrature_table[encoder->last_state][current_state];
    
    if (direction != 0) {
        // Calculate acceleration factor
        uint8_t step = 1;
        
        if (encoder->acceleration_enabled) {
            uint32_t time_diff = current_time - encoder->last_rotation_time;
            
            if (time_diff > 0) {
                // Calculate RPM (rotations per minute)
                // Assuming 20 pulses per rotation (typical for KY-040)
                uint32_t rpm = (60000 / time_diff) / 20;
                
                if (rpm > ROTARY_ACCEL_THRESHOLD_RPM) {
                    // Acceleration: 2x, 4x, 8x based on speed
                    if (rpm > 200) {
                        encoder->acceleration_factor = 8;
                    } else if (rpm > 120) {
                        encoder->acceleration_factor = 4;
                    } else {
                        encoder->acceleration_factor = 2;
                    }
                } else {
                    encoder->acceleration_factor = 1;
                }
            }
            
            step = encoder->acceleration_factor;
        }
        
        // Update position
        if (direction > 0) {
            encoder->position += step;
            encoder->direction = ROTARY_DIR_CW;
        } else {
            encoder->position -= step;
            encoder->direction = ROTARY_DIR_CCW;
        }
        
        // Apply limits
        if (encoder->use_limits) {
            if (encoder->position < encoder->min_position) {
                encoder->position = encoder->min_position;
            } else if (encoder->position > encoder->max_position) {
                encoder->position = encoder->max_position;
            }
        }
        
        // Update timestamp
        encoder->last_rotation_time = current_time;
        encoder->last_change_time = current_time;
        
        // Call rotation callback
        if (encoder->on_rotate) {
            encoder->on_rotate(encoder->position, encoder->direction);
        }
    }
    
    // Update last state
    encoder->last_state = current_state;
}

/**
 * @brief ค้นหา encoder instance จาก pin number
 *
 * @param pin - Pin number ที่ต้องการค้นหา
 * @return Pointer to RotaryEncoder หรือ NULL ถ้าไม่พบ
 *
 * @note ใช้ g_encoders[] global array (สูงสุด 8 instances)
 *       เทียบ pin_clk หรือ pin_dt ของแต่ละ encoder
 */
static RotaryEncoder* Rotary_FindEncoder(uint8_t pin) {
    for (uint8_t i = 0; i < g_encoder_count; i++) {
        if (g_encoders[i]->pin_clk == pin || g_encoders[i]->pin_dt == pin) {
            return g_encoders[i];
        }
    }
    return NULL;
}

/* ========== Global Interrupt Handlers ========== */

/**
 * @brief Global EXTI interrupt handler
 *
 * @param pin - pin number ที่เกิด interrupt
 *
 * @note ฟังก์ชันนี้ถูกเรียกจาก EXTI ISR ของ SimpleGPIO
 *       ต้อง link กับ interrupt system ของ MCU
 *       ค้นหา encoder ที่ตรงกับ pin แล้วเรียก ProcessRotation
 *       ทำงานใน ISR context — ห้ามใช้ blocking operation
 */
void Rotary_EXTI_Handler(uint8_t pin) {
    RotaryEncoder* encoder = Rotary_FindEncoder(pin);
    if (encoder) {
        Rotary_ProcessRotation(encoder);
    }
}
