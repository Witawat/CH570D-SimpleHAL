/**
 * @file RCWL0516.c
 * @brief RCWL-0516 Microwave Radar Motion Sensor Implementation
 */

#include "RCWL0516.h"
#include <string.h>

/* ========== Private helpers ========== */

/**
 * @brief ใส่ sample ใหม่เข้า filter buffer และคืน majority vote
 * @return 1 ถ้า majority เป็น HIGH
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

RCWL0516_Status RCWL0516_Init(RCWL0516_Instance* r, uint8_t out_pin) {
    RCWL0516_Config cfg;
    cfg.out_pin      = out_pin;
    cfg.cds_pin      = RCWL0516_PIN_NONE;
    cfg.hold_ms      = RCWL0516_DEFAULT_HOLD_MS;
    cfg.debounce_ms  = RCWL0516_DEFAULT_DEBOUNCE_MS;
    cfg.filter_count = RCWL0516_DEFAULT_FILTER_COUNT;
    return RCWL0516_InitWithConfig(r, &cfg);
}

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

uint8_t RCWL0516_IsMotionDetected(RCWL0516_Instance* r) {
    if (r == NULL || !r->initialized) return 0;
    return (r->state == RCWL0516_STATE_TRIGGERED  ||
            r->state == RCWL0516_STATE_ACTIVE      ||
            r->state == RCWL0516_STATE_HOLDING) ? 1U : 0U;
}

uint8_t RCWL0516_ReadRaw(RCWL0516_Instance* r) {
    if (r == NULL || !r->initialized) return 0;
    return (uint8_t)(digitalRead(r->cfg.out_pin) == HIGH ? 1U : 0U);
}

RCWL0516_State RCWL0516_GetState(RCWL0516_Instance* r) {
    if (r == NULL || !r->initialized) return RCWL0516_STATE_IDLE;
    return r->state;
}

uint32_t RCWL0516_GetTotalTriggers(RCWL0516_Instance* r) {
    if (r == NULL || !r->initialized) return 0;
    return r->total_triggers;
}

uint32_t RCWL0516_GetLastDurationMs(RCWL0516_Instance* r) {
    if (r == NULL || !r->initialized) return 0;
    return r->last_duration_ms;
}

uint32_t RCWL0516_GetMsSinceLastTrigger(RCWL0516_Instance* r) {
    if (r == NULL || !r->initialized || r->trigger_ms == 0) return 0;
    return (Get_CurrentMs() - r->trigger_ms);
}

/* ========== Config ========== */

RCWL0516_Status RCWL0516_SetHoldTime(RCWL0516_Instance* r, uint32_t hold_ms) {
    if (r == NULL || !r->initialized) return RCWL0516_ERROR_NOT_INIT;
    r->cfg.hold_ms = hold_ms;
    return RCWL0516_OK;
}

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
