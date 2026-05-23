/**
 * @file SimplePIR.c
 * @brief Simple PIR Sensor Library Implementation
 * @version 1.0
 * @date 2025-12-21
 */

#include "PIR.h"
#include <string.h>

/* ========== Private Variables ========== */

static PIR_Instance pir_instances[PIR_MAX_INSTANCES];
static uint8_t pir_instance_count = 0;

/* ========== Private Function Prototypes ========== */

static bool PIR_ApplyFilter(PIR_Instance* pir, bool raw_value);
static void PIR_UpdateStateMachine(PIR_Instance* pir, bool filtered_value);
static PIR_Instance* PIR_AllocateInstance(void);

/* ========== Initialization Functions ========== */

/**
 * @brief เริ่มต้นการใช้งาน PIR sensor
 * @param pin - GPIO pin (PC0-PC7, PD2-PD7)
 * @return ตัวชี้ไปยัง PIR_Instance หรือ NULL ถ้า instance เต็ม
 * @note  ใช้ config default:
 *        debounce=150ms, blanking=200ms, timeout=5000ms
 *        filter=PIR_FILTER_MEDIUM, mode=PIR_MODE_CONTINUOUS
 *         ใช้ PIN_MODE_INPUT_PULLUP สำหรับ GPIO
 *        ต้องเรียก Timer_Init() ก่อน
 */
PIR_Instance* PIR_Init(uint8_t pin) {
    PIR_Config default_config = {
        .pin = pin,
        .debounce_ms = 150,
        .blanking_window_ms = 200,
        .timeout_ms = 5000,
        .filter_level = PIR_FILTER_MEDIUM,
        .mode = PIR_MODE_CONTINUOUS,
        .led_protection_enabled = false
    };
    
    return PIR_InitWithConfig(&default_config);
}

/**
 * @brief เริ่มต้น PIR พร้อม configuration
 * @param config - ตัวชี้ไปยัง PIR_Config (NULL → return NULL)
 * @return ตัวชี้ไปยัง PIR_Instance หรือ NULL ถ้า instance เต็ม
 * @note  จัดสรร instance จาก static pool (สูงสุด PIR_MAX_INSTANCES=4)
 *        คัดลอก config ด้วย memcpy
 *        ตั้งค่าเริ่มต้น: state=IDLE, current_value=false
 *        ใช้ PIN_MODE_INPUT_PULLUP สำหรับ GPIO
 */
PIR_Instance* PIR_InitWithConfig(PIR_Config* config) {
    if (config == NULL) return NULL;
    
    // Allocate instance
    PIR_Instance* pir = PIR_AllocateInstance();
    if (pir == NULL) return NULL;
    
    // Copy configuration
    memcpy(&pir->config, config, sizeof(PIR_Config));
    
    // Initialize GPIO
    pinMode(config->pin, PIN_MODE_INPUT_PULLUP);
    
    // Initialize state
    pir->state = PIR_STATE_IDLE;
    pir->current_value = false;
    pir->last_stable_value = false;
    
    // Initialize timing
    pir->last_change_time = Get_CurrentMs();
    pir->motion_start_time = 0;
    pir->motion_end_time = 0;
    pir->last_blanking_time = 0;
    
    // Initialize filter
    memset(pir->filter_buffer, 0, sizeof(pir->filter_buffer));
    pir->filter_index = 0;
    pir->filter_count = 0;
    
    // Initialize callbacks
    pir->on_motion_start = NULL;
    pir->on_motion_end = NULL;
    pir->on_motion_active = NULL;
    
    // Set flags
    pir->initialized = true;
    pir->in_blanking_window = false;
    
    return pir;
}

/* ========== Configuration Functions ========== */

/**
 * @brief ตั้งค่า debounce time
 * @param pir - instance (NULL → return ทันที)
 * @param ms  - เวลา debounce (milliseconds)
 * @note  debounce กรองสัญญาณกระเด้งของ PIR sensor
 *        ค่า default = 150ms (ตั้งใน PIR_Init)
 */
void PIR_SetDebounce(PIR_Instance* pir, uint16_t ms) {
    if (pir == NULL) return;
    pir->config.debounce_ms = ms;
}

/**
 * @brief ตั้งค่า blanking window
 * @param pir - instance (NULL → return ทันที)
 * @param ms  - เวลา blanking window (milliseconds)
 * @note  ป้องกันการรบกวนจาก LED ที่อาจทำให้ PIR อ่านผิดพลาด
 *        ระหว่าง blanking PIR จะไม่อ่านค่า
 *        ค่า default = 200ms (ตั้งใน PIR_Init)
 */
void PIR_SetBlankingWindow(PIR_Instance* pir, uint16_t ms) {
    if (pir == NULL) return;
    pir->config.blanking_window_ms = ms;
}

/**
 * @brief ตั้งค่า timeout
 * @param pir - instance (NULL → return ทันที)
 * @param ms  - เวลา timeout (milliseconds)
 * @note  timeout สำหรับ continuous detection
 *        หลังจาก signal หาย รอ timeout ก่อนเปลี่ยนเป็น IDLE
 *        ค่า default = 5000ms (ตั้งใน PIR_Init)
 */
void PIR_SetTimeout(PIR_Instance* pir, uint16_t ms) {
    if (pir == NULL) return;
    pir->config.timeout_ms = ms;
}

/**
 * @brief ตั้งค่า filter level
 * @param pir   - instance (NULL → return ทันที)
 * @param level - ระดับการกรอง (PIR_FILTER_NONE/LOW/MEDIUM/HIGH)
 * @note  filter ใช้ majority vote (threshold 50%)
 *        LOW=2 samples, MEDIUM=4, HIGH=8
 */
void PIR_SetFilterLevel(PIR_Instance* pir, PIR_FilterLevel level) {
    if (pir == NULL) return;
    pir->config.filter_level = level;
}

/**
 * @brief ตั้งค่า detection mode
 * @param pir  - instance (NULL → return ทันที)
 * @param mode - PIR_MODE_SINGLE หรือ PIR_MODE_CONTINUOUS
 * @note  SINGLE: ตรวจจับครั้งเดียวแล้วรีเซ็ต
 *        CONTINUOUS: ตรวจจับต่อเนื่อง (default)
 */
void PIR_SetMode(PIR_Instance* pir, PIR_Mode mode) {
    if (pir == NULL) return;
    pir->config.mode = mode;
}

/**
 * @brief เปิด/ปิดการป้องกันการรบกวนจาก LED
 * @param pir     - instance (NULL → return ทันที)
 * @param enabled - true = เปิด, false = ปิด
 * @note  ต้องเรียก PIR_TriggerBlankingWindow() เมื่อ LED เปิด/ปิด
 *        default = false (ปิด) ใน PIR_Init
 */
void PIR_EnableLEDProtection(PIR_Instance* pir, bool enabled) {
    if (pir == NULL) return;
    pir->config.led_protection_enabled = enabled;
}

/* ========== Callback Functions ========== */

/**
 * @brief ตั้งค่า callback functions
 * @param pir     - instance (NULL → return ทันที)
 * @param on_start - เรียกเมื่อเริ่มตรวจพบการเคลื่อนไหว (NULL ได้)
 * @param on_end  - เรียกเมื่อการเคลื่อนไหวสิ้นสุด (NULL ได้)
 * @note  callback ถูกเรียกจาก PIR_UpdateStateMachine()
 *        on_start เรียกใน IDLE→MOTION_DETECTED
 *        on_end    เรียกใน MOTION_END→IDLE
 */
void PIR_SetCallback(PIR_Instance* pir, void (*on_start)(void), void (*on_end)(void)) {
    if (pir == NULL) return;
    pir->on_motion_start = on_start;
    pir->on_motion_end = on_end;
}

/**
 * @brief ตั้งค่า callback สำหรับ motion active
 * @param pir      - instance (NULL → return ทันที)
 * @param on_active - เรียกขณะมีการเคลื่อนไหว (NULL ได้)
 * @note  เรียกซ้ำทุกรอบที่ PIR_Update() และ state=ACTIVE
 */
void PIR_SetActiveCallback(PIR_Instance* pir, void (*on_active)(void)) {
    if (pir == NULL) return;
    pir->on_motion_active = on_active;
}

/* ========== Core Functions ========== */

/**
 * @brief อัพเดทสถานะของ PIR (เรียกใน main loop)
 * @param pir - instance (NULL หรือไม่ initialized → return ทันที)
 * @note  ต้องเรียกฟังก์ชันนี้ใน main loop ทุกรอบ
 *        ลำดับ: check blanking → raw → filter → debounce → state machine
 *        ใช้ ELAPSED_TIME() สำหรับ timing
 *        ถ้าอยู่ใน blanking window จะไม่อ่านค่า PIR
 */
void PIR_Update(PIR_Instance* pir) {
    if (pir == NULL || !pir->initialized) return;
    
    uint32_t current_time = Get_CurrentMs();
    
    // ตรวจสอบ blanking window
    if (pir->in_blanking_window) {
        if (ELAPSED_TIME(pir->last_blanking_time, current_time) >= pir->config.blanking_window_ms) {
            pir->in_blanking_window = false;
        } else {
            // ยังอยู่ใน blanking window - ไม่อ่านค่า PIR
            return;
        }
    }
    
    // อ่านค่า raw
    bool raw_value = PIR_ReadRaw(pir);
    
    // ผ่าน filter
    bool filtered_value = PIR_ApplyFilter(pir, raw_value);
    
    // Debounce
    if (filtered_value != pir->last_stable_value) {
        if (ELAPSED_TIME(pir->last_change_time, current_time) >= pir->config.debounce_ms) {
            pir->last_stable_value = filtered_value;
            pir->last_change_time = current_time;
            pir->current_value = filtered_value;
        }
    } else {
        pir->last_change_time = current_time;
    }
    
    // อัพเดท state machine
    PIR_UpdateStateMachine(pir, pir->current_value);
}

/**
 * @brief อ่านค่า PIR ปัจจุบัน (หลังผ่าน debounce และ filter)
 * @param pir - instance (NULL → return false)
 * @return true = ตรวจพบการเคลื่อนไหว, false = ไม่พบ
 * @note  คืนค่า current_value ซึ่งเป็น stable value หลัง debounce
 *        ไม่ได้อ่าน GPIO โดยตรง
 */
bool PIR_Read(PIR_Instance* pir) {
    if (pir == NULL) return false;
    return pir->current_value;
}

/**
 * @brief อ่านค่า PIR แบบ raw (ไม่ผ่าน debounce/filter)
 * @param pir - instance (NULL → return false)
 * @return true = HIGH (motion sensor active), false = LOW
 * @note  อ่านจาก GPIO โดยตรงผ่าน digitalRead()
 *        ไม่ผ่าน debounce, filter, หรือ blanking window
 */
bool PIR_ReadRaw(PIR_Instance* pir) {
    if (pir == NULL) return false;
    return digitalRead(pir->config.pin) == HIGH;
}

/* ========== State Query Functions ========== */

/**
 * @brief อ่านสถานะปัจจุบัน
 * @param pir - instance (NULL → return PIR_STATE_IDLE)
 * @return PIR_State ปัจจุบัน
 */
PIR_State PIR_GetState(PIR_Instance* pir) {
    if (pir == NULL) return PIR_STATE_IDLE;
    return pir->state;
}

/**
 * @brief ตรวจสอบว่ามีการเคลื่อนไหวหรือไม่
 * @param pir - instance (NULL → return false)
 * @return true = state เป็น MOTION_DETECTED หรือ MOTION_ACTIVE
 * @note  ไม่นับ MOTION_TIMEOUT หรือ MOTION_END
 */
bool PIR_IsMotionDetected(PIR_Instance* pir) {
    if (pir == NULL) return false;
    return (pir->state == PIR_STATE_MOTION_DETECTED || 
            pir->state == PIR_STATE_MOTION_ACTIVE);
}

/**
 * @brief อ่านระยะเวลาที่ตรวจพบการเคลื่อนไหว
 * @param pir - instance (NULL หรือ motion_start_time=0 → return 0)
 * @return ระยะเวลา (milliseconds)
 * @note  ถ้ายัง active → คำนวณจาก motion_start_time ถึง current time
 *        ถ้าจบแล้ว → คำนวณจาก motion_start_time ถึง motion_end_time
 */
uint32_t PIR_GetMotionDuration(PIR_Instance* pir) {
    if (pir == NULL) return 0;
    if (pir->motion_start_time == 0) return 0;
    
    if (pir->state == PIR_STATE_MOTION_ACTIVE || pir->state == PIR_STATE_MOTION_DETECTED) {
        return ELAPSED_TIME(pir->motion_start_time, Get_CurrentMs());
    } else if (pir->motion_end_time > 0) {
        return ELAPSED_TIME(pir->motion_start_time, pir->motion_end_time);
    }
    
    return 0;
}

/**
 * @brief อ่านเวลาที่ผ่านไปนับจากการเคลื่อนไหวครั้งล่าสุด
 * @param pir - instance (NULL หรือ motion_end_time=0 → return 0)
 * @return เวลาที่ผ่านไปนับจาก motion_end_time (milliseconds)
 * @note  ถ้าไม่เคยมีการเคลื่อนไหว → return 0
 */
uint32_t PIR_GetTimeSinceLastMotion(PIR_Instance* pir) {
    if (pir == NULL) return 0;
    if (pir->motion_end_time == 0) return 0;
    return ELAPSED_TIME(pir->motion_end_time, Get_CurrentMs());
}

/* ========== LED Interference Prevention ========== */

/**
 * @brief เรียกใช้ blanking window (เมื่อ LED เปิด/ปิด)
 * @param pir - instance (NULL → return ทันที)
 * @note  PIR จะไม่อ่านค่าในช่วง blanking window
 *        ต้องเปิด led_protection_enabled ก่อน
 *        เรียกทันทีหลังจาก LED เปิด/ปิด
 */
void PIR_TriggerBlankingWindow(PIR_Instance* pir) {
    if (pir == NULL) return;
    if (!pir->config.led_protection_enabled) return;
    
    pir->in_blanking_window = true;
    pir->last_blanking_time = Get_CurrentMs();
}

/**
 * @brief ตรวจสอบว่าอยู่ใน blanking window หรือไม่
 * @param pir - instance (NULL → return false)
 * @return true = อยู่ใน blanking window, false = ไม่อยู่
 * @note  PIR_Update() จะออกจาก blanking window อัตโนมัติเมื่อครบเวลา
 */
bool PIR_IsInBlankingWindow(PIR_Instance* pir) {
    if (pir == NULL) return false;
    return pir->in_blanking_window;
}

/* ========== Advanced Functions ========== */

/**
 * @brief รีเซ็ตสถานะของ PIR
 * @param pir - instance (NULL → return ทันที)
 * @note  รีเซ็ต state=IDLE, clear ค่า timing, clear filter buffer
 *        ไม่เปลี่ยนแปลง config
 */
void PIR_Reset(PIR_Instance* pir) {
    if (pir == NULL) return;
    
    pir->state = PIR_STATE_IDLE;
    pir->current_value = false;
    pir->last_stable_value = false;
    pir->motion_start_time = 0;
    pir->motion_end_time = 0;
    pir->in_blanking_window = false;
    
    memset(pir->filter_buffer, 0, sizeof(pir->filter_buffer));
    pir->filter_index = 0;
    pir->filter_count = 0;
}

/**
 * @brief อ่านค่าจาก filter buffer (สำหรับ debugging)
 * @param pir - instance (NULL หรือ filter_count=0 → return 0)
 * @return ค่าที่กรองแล้ว normalized 0-255
 * @note  (sum / count) * 255 — ใช้อ่านอัตราส่วน 1s ใน buffer
 */
uint8_t PIR_GetFilteredValue(PIR_Instance* pir) {
    if (pir == NULL) return 0;
    
    uint8_t sum = 0;
    uint8_t count = pir->filter_count;
    
    if (count == 0) return 0;
    
    for (uint8_t i = 0; i < count; i++) {
        sum += pir->filter_buffer[i];
    }
    
    return (sum * 255) / count;
}

/* ========== Utility Functions ========== */

/**
 * @brief อัพเดท PIR instances ทั้งหมด
 * @note  เรียก PIR_Update() สำหรับทุก instance ที่ initialized
 *        สะดวกเมื่อใช้ PIR หลายตัว
 */
void PIR_UpdateAll(void) {
    for (uint8_t i = 0; i < pir_instance_count; i++) {
        if (pir_instances[i].initialized) {
            PIR_Update(&pir_instances[i]);
        }
    }
}

/**
 * @brief หา PIR instance จาก pin
 * @param pin - GPIO pin
 * @return ตัวชี้ไปยัง PIR_Instance หรือ NULL ถ้าไม่พบ
 * @note  ค้นหาใน static pir_instances[] ที่ initialized แล้ว
 */
PIR_Instance* PIR_GetInstanceByPin(uint8_t pin) {
    for (uint8_t i = 0; i < pir_instance_count; i++) {
        if (pir_instances[i].initialized && pir_instances[i].config.pin == pin) {
            return &pir_instances[i];
        }
    }
    return NULL;
}

/* ========== Private Functions ========== */

/**
 * @brief ใช้ filter กับค่าที่อ่านได้
 * @param pir       - instance
 * @param raw_value - ค่า raw (true=HIGH)
 * @return bool ค่าหลัง filter (majority vote; threshold 50%)
 * @note  buffer เป็น circular buffer
 *        PIR_FILTER_NONE → ไม่กรอง ส่ง raw_value ผ่าน
 *        LOW=2, MEDIUM=4, HIGH=8 samples
 */
static bool PIR_ApplyFilter(PIR_Instance* pir, bool raw_value) {
    if (pir->config.filter_level == PIR_FILTER_NONE) {
        return raw_value;
    }
    
    // กำหนดจำนวน samples ตาม filter level
    uint8_t max_samples;
    switch (pir->config.filter_level) {
        case PIR_FILTER_LOW:    max_samples = 2; break;
        case PIR_FILTER_MEDIUM: max_samples = 4; break;
        case PIR_FILTER_HIGH:   max_samples = 8; break;
        default:                max_samples = 1; break;
    }
    
    // เพิ่มค่าใหม่เข้า buffer
    pir->filter_buffer[pir->filter_index] = raw_value ? 1 : 0;
    pir->filter_index = (pir->filter_index + 1) % max_samples;
    
    if (pir->filter_count < max_samples) {
        pir->filter_count++;
    }
    
    // คำนวณค่าเฉลี่ย
    uint8_t sum = 0;
    for (uint8_t i = 0; i < pir->filter_count; i++) {
        sum += pir->filter_buffer[i];
    }
    
    // ใช้ threshold 50%
    return (sum * 2) >= pir->filter_count;
}

/**
 * @brief อัพเดท state machine
 * @param pir             - instance
 * @param filtered_value  - ค่าหลัง filter (true=มี motion)
 * @note  state flow: IDLE → DETECTED → ACTIVE → TIMEOUT → END → IDLE
 *        ถ้า signal หายใน DETECTED → false alarm กลับ IDLE
 *        ใช้ ELAPSED_TIME() เช็ค timeout
 *        SINGLE mode: หลัง END → รีเซ็ตด้วย PIR_Reset()
 */
static void PIR_UpdateStateMachine(PIR_Instance* pir, bool filtered_value) {
    uint32_t current_time = Get_CurrentMs();
    
    switch (pir->state) {
        case PIR_STATE_IDLE:
            if (filtered_value) {
                pir->state = PIR_STATE_MOTION_DETECTED;
                pir->motion_start_time = current_time;
                
                // Callback
                if (pir->on_motion_start != NULL) {
                    pir->on_motion_start();
                }
            }
            break;
            
        case PIR_STATE_MOTION_DETECTED:
            if (filtered_value) {
                pir->state = PIR_STATE_MOTION_ACTIVE;
                
                // Callback
                if (pir->on_motion_active != NULL) {
                    pir->on_motion_active();
                }
            } else {
                // False alarm - กลับไป IDLE
                pir->state = PIR_STATE_IDLE;
                pir->motion_start_time = 0;
            }
            break;
            
        case PIR_STATE_MOTION_ACTIVE:
            if (filtered_value) {
                // ยังมีการเคลื่อนไหว - รีเซ็ต timeout
                pir->motion_end_time = current_time;
                
                // Callback (ถ้าต้องการ)
                if (pir->on_motion_active != NULL) {
                    pir->on_motion_active();
                }
            } else {
                // หยุดเคลื่อนไหว - เริ่ม timeout
                pir->state = PIR_STATE_MOTION_TIMEOUT;
                pir->motion_end_time = current_time;
            }
            break;
            
        case PIR_STATE_MOTION_TIMEOUT:
            if (filtered_value) {
                // มีการเคลื่อนไหวอีก - กลับไป ACTIVE
                pir->state = PIR_STATE_MOTION_ACTIVE;
            } else {
                // ตรวจสอบ timeout
                if (ELAPSED_TIME(pir->motion_end_time, current_time) >= pir->config.timeout_ms) {
                    pir->state = PIR_STATE_MOTION_END;
                }
            }
            break;
            
        case PIR_STATE_MOTION_END:
            // Callback
            if (pir->on_motion_end != NULL) {
                pir->on_motion_end();
            }
            
            // รีเซ็ตและกลับไป IDLE
            pir->state = PIR_STATE_IDLE;
            pir->motion_start_time = 0;
            pir->motion_end_time = 0;
            
            // ถ้าเป็น single mode - รีเซ็ต
            if (pir->config.mode == PIR_MODE_SINGLE) {
                PIR_Reset(pir);
            }
            break;
    }
}

/**
 * @brief จัดสรร PIR instance ใหม่จาก static pool
 * @return ตัวชี้ไปยัง PIR_Instance หรือ NULL ถ้าเต็ม (PIR_MAX_INSTANCES)
 * @note  ใช้ static array pir_instances[]
 *        clear instance ด้วย memset(..., 0, ...)
 */
static PIR_Instance* PIR_AllocateInstance(void) {
    if (pir_instance_count >= PIR_MAX_INSTANCES) {
        return NULL;
    }
    
    PIR_Instance* pir = &pir_instances[pir_instance_count];
    pir_instance_count++;
    
    // Clear instance
    memset(pir, 0, sizeof(PIR_Instance));
    
    return pir;
}
