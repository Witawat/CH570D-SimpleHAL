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

int32_t Rotary_GetPosition(RotaryEncoder* encoder) {
    return encoder->position;
}

void Rotary_SetPosition(RotaryEncoder* encoder, int32_t position) {
    encoder->position = position;
    encoder->last_position = position;
}

RotaryDirection Rotary_GetDirection(RotaryEncoder* encoder) {
    return encoder->direction;
}

bool Rotary_HasChanged(RotaryEncoder* encoder) {
    if (encoder->position != encoder->last_position) {
        encoder->last_position = encoder->position;
        return true;
    }
    return false;
}

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

void Rotary_ClearLimits(RotaryEncoder* encoder) {
    encoder->use_limits = false;
}

/* ========== Button Control Functions ========== */

bool Rotary_IsButtonPressed(RotaryEncoder* encoder) {
    return (digitalRead(encoder->pin_sw) == LOW);  // Active LOW
}

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

void Rotary_SetDebounceTime(RotaryEncoder* encoder, uint16_t debounce_ms) {
    encoder->debounce_ms = debounce_ms;
}

void Rotary_SetAcceleration(RotaryEncoder* encoder, bool enabled) {
    encoder->acceleration_enabled = enabled;
    if (!enabled) {
        encoder->acceleration_factor = 1;
    }
}

/* ========== Callback Functions ========== */

void Rotary_OnRotate(RotaryEncoder* encoder, void (*callback)(int32_t, RotaryDirection)) {
    encoder->on_rotate = callback;
}

void Rotary_OnButtonPress(RotaryEncoder* encoder, void (*callback)(void)) {
    encoder->on_button_press = callback;
}

void Rotary_OnButtonRelease(RotaryEncoder* encoder, void (*callback)(void)) {
    encoder->on_button_release = callback;
}

void Rotary_OnButtonLongPress(RotaryEncoder* encoder, void (*callback)(void)) {
    encoder->on_button_long_press = callback;
}

void Rotary_OnButtonDoubleClick(RotaryEncoder* encoder, void (*callback)(void)) {
    encoder->on_button_double_click = callback;
}

/* ========== Core Processing Functions ========== */

void Rotary_Update(RotaryEncoder* encoder) {
    Rotary_ProcessRotation(encoder);
}

void Rotary_CLK_ISR(RotaryEncoder* encoder) {
    Rotary_ProcessRotation(encoder);
}

void Rotary_DT_ISR(RotaryEncoder* encoder) {
    Rotary_ProcessRotation(encoder);
}

/* ========== Private Functions ========== */

/**
 * @brief อ่าน state ของ encoder (2-bit)
 * @param encoder Pointer to RotaryEncoder structure
 * @return State (0-3): bit1=CLK, bit0=DT
 */
static uint8_t Rotary_ReadState(RotaryEncoder* encoder) {
    uint8_t clk = digitalRead(encoder->pin_clk);
    uint8_t dt = digitalRead(encoder->pin_dt);
    return (clk << 1) | dt;
}

/**
 * @brief ประมวลผลการหมุน encoder
 * @param encoder Pointer to RotaryEncoder structure
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
 * @param pin Pin number
 * @return Pointer to encoder หรือ NULL
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
 * @note ฟังก์ชันนี้ถูกเรียกจาก EXTI ISR ของ SimpleGPIO
 * @note ต้อง link กับ interrupt system ของ MCU
 */
void Rotary_EXTI_Handler(uint8_t pin) {
    RotaryEncoder* encoder = Rotary_FindEncoder(pin);
    if (encoder) {
        Rotary_ProcessRotation(encoder);
    }
}
