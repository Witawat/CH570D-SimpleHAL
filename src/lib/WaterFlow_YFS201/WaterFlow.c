/**
 * @file WaterFlow.c
 * @brief YF-S201 Water Flow Sensor Library Implementation
 * @version 1.0
 * @date 2026-05-01
 */

#include "WaterFlow.h"

/* ========== Private Globals ========== */

static WaterFlow_Instance* g_flow_instances[WATERFLOW_MAX_INSTANCES] = {NULL};
static uint8_t g_flow_count = 0;

/* ========== Private Helpers ========== */

/* ========== ISR Handlers ========== */

/**
 * @brief ISR callback สำหรับ pulse counting
 * @return None
 * @note Called from EXTI interrupt on RISING edge. Iterates through all
 *       registered sensor instances and increments pulse_count for the
 *       matching pin. The pulse_count must be read atomically with
 *       __disable_irq()/__enable_irq() from the main loop to avoid
 *       race conditions. ISR execution time scales with instance count.
 */
static void WaterFlow_PulseISR(void) {
    /* หา instance จาก pin ที่เกิด interrupt */
    /* Note: ใช้ polling approach — อ่าน pin state แล้ว match กับ instance */
    uint8_t i;
    for (i = 0; i < g_flow_count; i++) {
        WaterFlow_Instance* flow = g_flow_instances[i];
        if (flow == NULL || !flow->initialized) continue;

        /* check if this pin triggered (RISING edge) */
        if (digitalRead(flow->pin) == HIGH) {
            flow->pulse_count++;
        }
    }
}

/* ========== Public Functions ========== */

/**
 * @brief Initialize a water flow sensor instance
 * @param flow     Pointer to WaterFlow_Instance
 * @param pin      GPIO pin connected to the sensor output (with interrupt support)
 * @param k_factor Sensor K-factor (pulses per liter). Must be > 0.
 * @return WATERFLOW_OK on success, WATERFLOW_ERROR_PARAM if params invalid,
 *         WATERFLOW_ERROR_FULL if max instances (WATERFLOW_MAX_INSTANCES) reached
 * @note Configures the pin as input with pull-up and attaches a RISING edge
 *       interrupt. Registers the instance in the global list for ISR dispatch.
 *       The K-factor is sensor-specific (e.g., YF-S201 = 450 pulses/L).
 */
WaterFlow_Status WaterFlow_Init(WaterFlow_Instance* flow, uint8_t pin, float k_factor) {

    if (flow == NULL)     return WATERFLOW_ERROR_PARAM;
    if (k_factor <= 0.0f) return WATERFLOW_ERROR_PARAM;

    /* check max instances */
    if (g_flow_count >= WATERFLOW_MAX_INSTANCES) {
        return WATERFLOW_ERROR_FULL;
    }

    flow->pin          = pin;
    flow->k_factor     = k_factor;
    flow->pulse_count  = 0;
    flow->last_pulse   = 0;
    flow->last_time_ms = 0;
    flow->initialized  = 1;

    /* setup GPIO as input */
    pinMode(pin, PIN_MODE_INPUT_PULLUP);

    /* setup EXTI interrupt on RISING edge */
    attachInterrupt(pin, WaterFlow_PulseISR, RISING);

    /* register instance */
    g_flow_instances[g_flow_count++] = flow;

    return WATERFLOW_OK;
}

/**
 * @brief Get the raw pulse count since last reset
 * @param flow Pointer to WaterFlow_Instance
 * @return Total pulse count, or 0 if flow is NULL or uninitialized
 * @note Reads pulse_count atomically with interrupts disabled. The counter
 *       is incremented by the ISR on each sensor pulse.
 */
uint32_t WaterFlow_GetPulseCount(WaterFlow_Instance* flow) {
    uint32_t count;

    if (flow == NULL || !flow->initialized) return 0;

    __disable_irq();
    count = flow->pulse_count;
    __enable_irq();

    return count;
}

/**
 * @brief Calculate the current flow rate in L/min
 * @param flow Pointer to WaterFlow_Instance
 * @return Flow rate in liters per minute, or 0.0f if insufficient data
 * @note Computes rate = (delta_pulses / k_factor) / (delta_ms / 60000).
 *       Stores a baseline on the first call and computes differential on
 *       subsequent calls. Returns 0 on the first call or if delta is zero.
 */
float WaterFlow_GetFlowRate(WaterFlow_Instance* flow) {
    uint32_t current_pulses;
    uint32_t current_time_ms;
    uint32_t delta_pulses;
    uint32_t delta_ms;
    float flow_rate;

    if (flow == NULL || !flow->initialized) return 0.0f;

    current_time_ms = Get_CurrentMs();

    __disable_irq();
    current_pulses = flow->pulse_count;
    __enable_irq();

    /* first call — store baseline */
    if (flow->last_time_ms == 0) {
        flow->last_pulse   = current_pulses;
        flow->last_time_ms = current_time_ms;
        return 0.0f;
    }

    delta_pulses = current_pulses - flow->last_pulse;
    delta_ms     = current_time_ms - flow->last_time_ms;

    /* update baseline for next call */
    flow->last_pulse   = current_pulses;
    flow->last_time_ms = current_time_ms;

    if (delta_ms == 0 || delta_pulses == 0) return 0.0f;

    /* Calculate flow rate: (pulses / k_factor) / (ms / 60000) = L/min */
    flow_rate = ((float)delta_pulses / flow->k_factor) / ((float)delta_ms / 60000.0f);

    return flow_rate;
}

/**
 * @brief Get total accumulated volume in liters
 * @param flow Pointer to WaterFlow_Instance
 * @return Total volume in liters (pulses / k_factor), or 0.0f on error
 * @note Reads pulse_count atomically and divides by the K-factor. The total
 *       is reset when WaterFlow_Reset is called.
 */
float WaterFlow_GetTotalVolume(WaterFlow_Instance* flow) {
    uint32_t pulses;

    if (flow == NULL || !flow->initialized) return 0.0f;

    __disable_irq();
    pulses = flow->pulse_count;
    __enable_irq();

    if (flow->k_factor <= 0.0f) return 0.0f;

    return (float)pulses / flow->k_factor;
}

/**
 * @brief Reset pulse count and flow rate baseline
 * @param flow Pointer to WaterFlow_Instance
 * @return None
 * @note Atomically clears pulse_count and resets the flow rate baseline
 *       (last_pulse and last_time_ms). Total volume effectively resets to zero.
 */
void WaterFlow_Reset(WaterFlow_Instance* flow) {
    if (flow == NULL || !flow->initialized) return;

    __disable_irq();
    flow->pulse_count  = 0;
    __enable_irq();

    flow->last_pulse   = 0;
    flow->last_time_ms = 0;
}

/**
 * @brief Update the K-factor calibration value
 * @param flow     Pointer to WaterFlow_Instance
 * @param k_factor New K-factor (pulses per liter). Must be > 0.
 * @return None
 * @note Affects all subsequent flow rate and volume calculations. Common
 *       values: YF-S201 = 450, YF-S201C = 5880.
 */
void WaterFlow_SetKFactor(WaterFlow_Instance* flow, float k_factor) {
    if (flow == NULL || !flow->initialized) return;
    if (k_factor <= 0.0f) return;
    flow->k_factor = k_factor;
}
