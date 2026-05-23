/**
 * @file RCWL0516.c
 * @brief RCWL-0516 Microwave Radar Motion Sensor Implementation
 */

#include "RCWL0516.h"
#include <string.h>

/* ========== Private helpers ========== */

/**
 * @brief ใส่ sample ใหม่เข้า filter buffer และคืน majority vote
 * @param r   - instance
 * @param val - sample ใหม่ (0/1)
 * @return 1 ถ้า majority (เกินครึ่ง) ของ buffer เป็น HIGH, 0 ถ้าไม่ใช่
 * @note  buffer เป็น circular ขนาด filter_count (1-8)
 *        majority: ones > depth/2
 *        depth น้อยกว่า 1 หรือมากกว่า 8 จะถูก clamp
 */
static uint8_t _filter_push(RCWL0516_Instance* r, uint8_t val) {
    uint8_t depth = r->cfg.filter_count;
    if (depth < 1) depth = 1;
    if (depth > 8) depth = 8;

    r->filter_buf[r->filter_idx] = val;
    r->filter_idx = (uint8_t)((r->filter_idx + 1U) % depth);

    /* count 1s */
    uint8_t ones = 0;
    for (uint8_t i = 0; i < depth; ++i) {
        if (r->filter_buf[i]) ++ones;
    }
    return (ones > depth / 2) ? 1U : 0U;
}

/* ========== Init ========== */

/**
 * @brief เริ่มต้น sensor ด้วยค่า default
 * @param r       - instance (NULL → return RCWL0516_ERROR_PARAM)
 * @param out_pin - GPIO pin ต่อกับ OUT ของ RCWL-0516
 * @return RCWL0516_OK หรือ RCWL0516_ERROR_PARAM
 * @note  config default: hold=2000ms, debounce=50ms, filter=3
 *        cds_pin=RCWL0516_PIN_NONE (ไม่ใช้)
 *        อาศัย RCWL0516_InitWithConfig() ภายใน
 */
RCWL0516_Status RCWL0516_Init(RCWL0516_Instance* r, uint8_t out_pin) {
    RCWL0516_Config cfg;
    cfg.out_pin      = out_pin;
    cfg.cds_pin      = RCWL0516_PIN_NONE;
    cfg.hold_ms      = RCWL0516_DEFAULT_HOLD_MS;
    cfg.debounce_ms  = RCWL0516_DEFAULT_DEBOUNCE_MS;
    cfg.filter_count = RCWL0516_DEFAULT_FILTER_COUNT;
    return RCWL0516_InitWithConfig(r, &cfg);
}

/**
 * @brief เริ่มต้น sensor พร้อม config ที่กำหนดเอง
 * @param r   - instance (NULL → return RCWL0516_ERROR_PARAM)
 * @param cfg - config struct (NULL หรือ out_pin=NONE → return RCWL0516_ERROR_PARAM)
 * @return RCWL0516_OK หรือ RCWL0516_ERROR_PARAM
 * @note  ตั้ง GPIO: out_pin=input, cds_pin=input (ถ้าไม่ใช่ NONE)
 *        sanitize filter_count: clamp 1-8
 *        clear instance ด้วย memset(0)
 *        รีเซ็ต state=IDLE, last_change_ms=current
 */
RCWL0516_Status RCWL0516_InitWithConfig(RCWL0516_Instance* r,
                                         const RCWL0516_Config* cfg) {
    if (r == NULL || cfg == NULL)         return RCWL0516_ERROR_PARAM;
    if (cfg->out_pin == RCWL0516_PIN_NONE) return RCWL0516_ERROR_PARAM;

    memset(r, 0, sizeof(RCWL0516_Instance));
    r->cfg = *cfg;

    /* GPIO init */
    pinMode(r->cfg.out_pin, PIN_MODE_INPUT);

    /* CDS pin: ถ้าใช้งาน ต้องเป็น input (LDR ดึง high/low ภายนอก) */
    if (r->cfg.cds_pin != RCWL0516_PIN_NONE) {
        pinMode(r->cfg.cds_pin, PIN_MODE_INPUT);
    }

    /* sanitize filter depth */
    if (r->cfg.filter_count < 1) r->cfg.filter_count = 1;
    if (r->cfg.filter_count > 8) r->cfg.filter_count = 8;

    r->state       = RCWL0516_STATE_IDLE;
    r->last_change_ms = Get_CurrentMs();
    r->initialized = 1;
    return RCWL0516_OK;
}

/**
 * @brief ตั้ง callback functions
 * @param r            - instance (NULL หรือไม่ initialized → return RCWL0516_ERROR_NOT_INIT)
 * @param on_triggered - เรียกเมื่อ rising edge ใหม่ (NULL ได้)
 * @param on_active    - เรียกซ้ำขณะ ACTIVE/HOLDING (NULL ได้)
 * @param on_idle      - เรียกเมื่อกลับสู่ IDLE (NULL ได้)
 * @return RCWL0516_OK หรือ RCWL0516_ERROR_NOT_INIT
 * @note  callback ถูกเรียกจาก RCWL0516_Update() ภายใน state machine
 *        NULL pointer = ไม่เรียก callback นั้น
 */
RCWL0516_Status RCWL0516_SetCallbacks(RCWL0516_Instance* r,
                                       void (*on_triggered)(void),
                                       void (*on_active)(void),
                                       void (*on_idle)(void)) {
    if (r == NULL || !r->initialized) return RCWL0516_ERROR_NOT_INIT;
    r->on_triggered = on_triggered;
    r->on_active    = on_active;
    r->on_idle      = on_idle;
    return RCWL0516_OK;
}

/* ========== Core Update ========== */

/**
 * @brief อัปเดต state machine — เรียกใน main loop ทุกรอบ
 * @param r - instance (NULL หรือไม่ initialized → return ทันที)
 * @note  ไม่ blocking
 *        ลำดับ: read raw → majority filter → debounce → state machine
 *        state flow: IDLE → TRIGGERED → ACTIVE → HOLDING → IDLE
 *        ถ้า stable_signal=HIGH ระหว่าง HOLDING → retrigger กลับ ACTIVE
 *        COOLDOWN state สงวนไว้ ไม่ได้ใช้จริง
 */
void RCWL0516_Update(RCWL0516_Instance* r) {
    if (r == NULL || !r->initialized) return;

    uint32_t now = Get_CurrentMs();

    /* 1. อ่าน raw GPIO */
    uint8_t raw = (uint8_t)(digitalRead(r->cfg.out_pin) == HIGH ? 1U : 0U);
    r->raw_signal = raw;

    /* 2. ผ่าน majority filter */
    uint8_t filtered = _filter_push(r, raw);

    /* 3. Debounce: เปลี่ยน stable_signal เมื่อ filtered ตรงกันนานพอ */
    if (filtered != r->stable_signal) {
        if ((now - r->last_change_ms) >= r->cfg.debounce_ms) {
            r->stable_signal  = filtered;
            r->last_change_ms = now;
        }
        /* ยังอยู่ใน debounce — ไม่เปลี่ยน state ก่อน */
    } else {
        r->last_change_ms = now;   /* reset debounce timer */
    }

    /* 4. State machine */
    switch (r->state) {

        case RCWL0516_STATE_IDLE:
            if (r->stable_signal) {
                /* Rising edge → TRIGGERED */
                r->state        = RCWL0516_STATE_TRIGGERED;
                r->trigger_ms   = now;
                ++r->total_triggers;
                if (r->on_triggered) r->on_triggered();
            }
            break;

        case RCWL0516_STATE_TRIGGERED:
            /* ผ่าน 1 rount transition → ACTIVE */
            r->state = RCWL0516_STATE_ACTIVE;
            r->last_active_ms = now;
            if (r->on_active) r->on_active();
            break;

        case RCWL0516_STATE_ACTIVE:
            if (r->stable_signal) {
                /* signal ยัง HIGH: update last_active */
                r->last_active_ms = now;
                if (r->on_active) r->on_active();
            } else {
                /* signal ลง → เริ่ม software hold */
                r->state          = RCWL0516_STATE_HOLDING;
                r->hold_start_ms  = now;
                r->last_duration_ms = (now - r->trigger_ms);
            }
            break;

        case RCWL0516_STATE_HOLDING:
            if (r->stable_signal) {
                /* retrigger ระหว่าง hold → กลับไป ACTIVE */
                r->state = RCWL0516_STATE_ACTIVE;
                r->last_active_ms = now;
                ++r->total_triggers;
                if (r->on_triggered) r->on_triggered();
                break;
            }
            if ((now - r->hold_start_ms) >= r->cfg.hold_ms) {
                /* hold หมด → COOLDOWN/IDLE */
                r->state = RCWL0516_STATE_IDLE;
                if (r->on_idle) r->on_idle();
            } else {
                if (r->on_active) r->on_active();
            }
            break;

        case RCWL0516_STATE_COOLDOWN:
            /* ไม่ใช้ในปัจจุบัน reserved สำหรับ future extension */
            r->state = RCWL0516_STATE_IDLE;
            break;

        default:
            r->state = RCWL0516_STATE_IDLE;
            break;
    }
}

/* ========== Query functions ========== */

/**
 * @brief ตรวจสอบว่า active อยู่ไหม (TRIGGERED + ACTIVE + HOLDING)
 * @param r - instance (NULL หรือไม่ initialized → return 0)
 * @return 1 = มี motion หรืออยู่ใน hold, 0 = ไม่มี
 * @note  นับรวม HOLDING state ด้วย (ยังอยู่ใน software hold)
 */
uint8_t RCWL0516_IsMotionDetected(RCWL0516_Instance* r) {
    if (r == NULL || !r->initialized) return 0;
    return (r->state == RCWL0516_STATE_TRIGGERED  ||
            r->state == RCWL0516_STATE_ACTIVE      ||
            r->state == RCWL0516_STATE_HOLDING) ? 1U : 0U;
}

/**
 * @brief อ่าน signal raw (ไม่ผ่าน debounce/filter)
 * @param r - instance (NULL หรือไม่ initialized → return 0)
 * @return 1 = HIGH (motion), 0 = LOW
 * @note  อ่าน digitalRead() จาก out_pin โดยตรง
 */
uint8_t RCWL0516_ReadRaw(RCWL0516_Instance* r) {
    if (r == NULL || !r->initialized) return 0;
    return (uint8_t)(digitalRead(r->cfg.out_pin) == HIGH ? 1U : 0U);
}

/**
 * @brief อ่านสถานะ state machine
 * @param r - instance (NULL หรือไม่ initialized → return RCWL0516_STATE_IDLE)
 * @return RCWL0516_State ปัจจุบัน
 */
RCWL0516_State RCWL0516_GetState(RCWL0516_Instance* r) {
    if (r == NULL || !r->initialized) return RCWL0516_STATE_IDLE;
    return r->state;
}

/**
 * @brief อ่านจำนวนครั้ง trigger ทั้งหมดนับแต่ init
 * @param r - instance (NULL หรือไม่ initialized → return 0)
 * @return จำนวนครั้งที่ trigger ทั้งหมด
 * @note  trigger = rising edge ที่เข้าสู่ TRIGGERED state
 *        รวม retrigger ระหว่าง HOLDING ด้วย
 */
uint32_t RCWL0516_GetTotalTriggers(RCWL0516_Instance* r) {
    if (r == NULL || !r->initialized) return 0;
    return r->total_triggers;
}

/**
 * @brief อ่านระยะเวลาที่ ACTIVE ในครั้งล่าสุด (ms)
 * @param r - instance (NULL หรือไม่ initialized → return 0)
 * @return ระยะเวลา active ครั้งล่าสุด (ms)
 * @note  อัปเดตเมื่อ signal ลง (ACTIVE→HOLDING transition)
 *        duration = trigger_ms → hold_start_ms
 */
uint32_t RCWL0516_GetLastDurationMs(RCWL0516_Instance* r) {
    if (r == NULL || !r->initialized) return 0;
    return r->last_duration_ms;
}

/**
 * @brief อ่านเวลาที่ผ่านมาหลังจาก trigger ครั้งล่าสุด (ms)
 * @param r - instance (NULL หรือไม่ initialized หรือไม่เคย trigger → return 0)
 * @return เวลาที่ผ่านไปนับจาก trigger_ms (ms)
 */
uint32_t RCWL0516_GetMsSinceLastTrigger(RCWL0516_Instance* r) {
    if (r == NULL || !r->initialized || r->trigger_ms == 0) return 0;
    return (Get_CurrentMs() - r->trigger_ms);
}

/* ========== Config ========== */

/**
 * @brief ปรับ software hold time
 * @param r       - instance (NULL หรือไม่ initialized → return RCWL0516_ERROR_NOT_INIT)
 * @param hold_ms - ระยะเวลา hold (ms)
 * @return RCWL0516_OK หรือ RCWL0516_ERROR_NOT_INIT
 * @note  software hold time ต่อท้าย hardware hold ของ module
 *        ถ้าต้องการลด hardware hold ต้องปรับ RC บน PCB
 */
RCWL0516_Status RCWL0516_SetHoldTime(RCWL0516_Instance* r, uint32_t hold_ms) {
    if (r == NULL || !r->initialized) return RCWL0516_ERROR_NOT_INIT;
    r->cfg.hold_ms = hold_ms;
    return RCWL0516_OK;
}

/**
 * @brief รีเซ็ต state กลับสู่ IDLE ทันที (ใช้สำหรับ test หรือ force clear)
 * @param r - instance (NULL หรือไม่ initialized → return RCWL0516_ERROR_NOT_INIT)
 * @return RCWL0516_OK หรือ RCWL0516_ERROR_NOT_INIT
 * @note  clear state, signal, timing, filter buffer
 *        ไม่เปลี่ยน cfg หรือ callbacks
 *        ใช้ force clear เมื่อต้องการ ignore current state
 */
RCWL0516_Status RCWL0516_Reset(RCWL0516_Instance* r) {
    if (r == NULL || !r->initialized) return RCWL0516_ERROR_NOT_INIT;

    r->state        = RCWL0516_STATE_IDLE;
    r->raw_signal   = 0;
    r->stable_signal = 0;
    r->trigger_ms   = 0;
    r->hold_start_ms = 0;
    r->last_active_ms = 0;
    r->last_duration_ms = 0;
    r->last_change_ms = Get_CurrentMs();
    memset(r->filter_buf, 0, sizeof(r->filter_buf));
    r->filter_idx   = 0;
    return RCWL0516_OK;
}
