/**
 * @file HX711.c
 * @brief HX711 24-bit ADC Load Cell Library Implementation
 * @version 1.0
 * @date 2026-04-29
 */

#include "HX711.h"

/* ========== Private Functions ========== */

/**
 * @brief รอให้ DOUT = LOW (HX711 พร้อมอ่าน)
 * @param hx - instance ที่ผ่านการ init แล้ว
 * @return 1 = HX711 พร้อมอ่าน (DOUT = LOW), 0 = timeout
 * @note  polling แบบ blocking ด้วย Get_CurrentMs()
 *       timeout = HX711_READY_TIMEOUT_MS (default 1000ms)
 *       ไม่มีการปิด SCK เมื่อ timeout
 */
static uint8_t _wait_ready(HX711_Instance* hx) {
    uint32_t start = Get_CurrentMs();
    while (digitalRead(hx->pin_dout) != 0) {
        if ((Get_CurrentMs() - start) >= HX711_READY_TIMEOUT_MS) {
            return 0;
        }
    }
    return 1;
}

/**
 * @brief อ่านค่า 24-bit จาก HX711 พร้อม gain pulse
 * @param hx - instance ที่ผ่านการ init แล้ว
 * @return int32_t signed 24-bit (sign-extended), ไม่ตรวจสอบ error
 *
 * @details โปรโตคอล:
 *   - รอ DOUT = LOW (ready signal)
 *   - ส่ง 24 clock pulses อ่าน 1 bit ต่อ pulse (MSB first)
 *   - ส่ง gain_pulses เพิ่มเติม (1=Gain128, 2=Gain32, 3=Gain64)
 *
 *   Timing:
 *   SCK LOW → DOUT stable → SCK HIGH → อ่าน DOUT → SCK LOW
 *   ความถี่ max ≈ 1MHz ที่ AVDD=5V, 320kHz ที่ AVDD=3.3V
 *
 * @note ใช้ __disable_irq() ป้องกัน interrupt กวน timing ที่แม่นยำ
 *       delay 1µs ระหว่าง SCK transition
 *       sign-extend 24-bit → 32-bit ด้วย mask 0x800000
 */
static int32_t _read_raw(HX711_Instance* hx) {
    uint32_t raw = 0;
    uint8_t  gain_pulses = (uint8_t)hx->gain; /* 1, 2, หรือ 3 */

    __disable_irq();

    /* อ่าน 24 bits */
    for (int i = 0; i < 24; i++) {
        digitalWrite(hx->pin_sck, 1);
        Delay_Us(1);
        raw = (raw << 1) | (digitalRead(hx->pin_dout) ? 1 : 0);
        digitalWrite(hx->pin_sck, 0);
        Delay_Us(1);
    }

    /* ส่ง gain select pulses */
    for (int i = 0; i < gain_pulses; i++) {
        digitalWrite(hx->pin_sck, 1);
        Delay_Us(1);
        digitalWrite(hx->pin_sck, 0);
        Delay_Us(1);
    }

    __enable_irq();

    /* Sign extend จาก 24-bit → 32-bit */
    if (raw & 0x800000) {
        raw |= 0xFF000000;
    }

    return (int32_t)raw;
}

/* ========== Public Functions ========== */

/**
 * @brief เริ่มต้น HX711
 * @param hx       - instance (NULL → return HX711_ERROR_PARAM)
 * @param pin_dout - GPIO pin เชื่อมต่อ DOUT
 * @param pin_sck  - GPIO pin เชื่อมต่อ PD_SCK
 * @return HX711_OK หรือ HX711_ERROR_PARAM / HX711_ERROR_TIMEOUT
 * @note  ตั้ง pin mode: DOUT=input, SCK=output LOW
 *        Default gain = HX711_GAIN_128 (CH-A), tare_offset = 0, calibration = 1.0
 *        อ่าน dummy 1 ครั้งเพื่อ apply gain หลัง init
 *        SCK LOW = power up state
 */
HX711_Status HX711_Init(HX711_Instance* hx, GPIO_Pin pin_dout, GPIO_Pin pin_sck) {
    if (hx == NULL) return HX711_ERROR_PARAM;

    hx->pin_dout          = pin_dout;
    hx->pin_sck           = pin_sck;
    hx->gain              = HX711_GAIN_128;
    hx->tare_offset       = 0;
    hx->calibration_factor = 1.0f;
    hx->initialized       = 0;

    /* ตั้ง pin mode */
    pinMode(pin_dout, PIN_MODE_INPUT);
    pinMode(pin_sck,  PIN_MODE_OUTPUT);
    digitalWrite(pin_sck, 0);  /* SCK LOW = power up */

    hx->initialized = 1;

    /* อ่าน 1 ครั้งเพื่อ apply gain setting */
    int32_t dummy;
    if (!_wait_ready(hx)) return HX711_ERROR_TIMEOUT;
    _read_raw(hx);
    (void)dummy;

    return HX711_OK;
}

/**
 * @brief ตรวจสอบว่า HX711 พร้อมอ่านหรือยัง
 * @param hx - instance (NULL หรือไม่ initialized → return 0)
 * @return 1 = พร้อม (DOUT = LOW), 0 = ยังไม่พร้อมหรือ error
 * @note  non-blocking — ใช้ digitalRead ตรงๆ ไม่มีการรอ
 */
uint8_t HX711_IsReady(HX711_Instance* hx) {
    if (hx == NULL || !hx->initialized) return 0;
    return (digitalRead(hx->pin_dout) == 0) ? 1 : 0;
}

/**
 * @brief อ่านค่า ADC 24-bit แบบ raw
 * @param hx     - instance (NULL หรือไม่ initialized → return HX711_ERROR_PARAM)
 * @param result - pointer รับค่า signed 24-bit (NULL → return HX711_ERROR_PARAM)
 * @return HX711_OK หรือ HX711_ERROR_PARAM / HX711_ERROR_TIMEOUT
 * @note  blocking รอ DOUT=LOW สูงสุด HX711_READY_TIMEOUT_MS
 *        ใช้ _wait_ready() + _read_raw() ภายใน
 *        ระหว่าง _read_raw() ใช้ __disable_irq() เพื่อ timing แม่นยำ
 */
HX711_Status HX711_Read(HX711_Instance* hx, int32_t* result) {
    if (hx == NULL || !hx->initialized || result == NULL) return HX711_ERROR_PARAM;

    if (!_wait_ready(hx)) return HX711_ERROR_TIMEOUT;

    *result = _read_raw(hx);
    return HX711_OK;
}

/**
 * @brief ตั้งศูนย์ (Tare) — อ่านค่าเฉลี่ยหลายครั้งและเก็บเป็น offset
 * @param hx      - instance (NULL หรือไม่ initialized → return HX711_ERROR_PARAM)
 * @param samples - จำนวน samples สำหรับเฉลี่ย (1-HX711_MAX_SAMPLES, 0/เกิน → ใช้ 10)
 * @return HX711_OK หรือ HX711_ERROR_PARAM / HX711_ERROR_TIMEOUT
 * @note  samples=0 หรือเกิน HX711_MAX_SAMPLES จะถูกปรับเป็น 10
 *        ค่าเฉลี่ยใช้ int64_t ป้องกัน overflow
 *        อย่าวางสิ่งของบน scale ระหว่าง tare
 */
HX711_Status HX711_Tare(HX711_Instance* hx, uint8_t samples) {
    if (hx == NULL || !hx->initialized) return HX711_ERROR_PARAM;
    if (samples == 0 || samples > HX711_MAX_SAMPLES) samples = 10;

    int64_t sum = 0;
    for (uint8_t i = 0; i < samples; i++) {
        if (!_wait_ready(hx)) return HX711_ERROR_TIMEOUT;
        sum += _read_raw(hx);
    }

    hx->tare_offset = (int32_t)(sum / samples);
    return HX711_OK;
}

/**
 * @brief ตั้งค่า calibration factor
 * @param hx     - instance (NULL หรือไม่ initialized → return ทันที)
 * @param factor - จำนวน raw units ต่อ 1 กรัม (0.0f → return ทันที ป้องกัน div/0)
 * @note  ถ้า factor == 0.0f จะ return ทันที ป้องกัน division by zero
 *        calibration_factor = raw_value / weight(g)
 *        เช่น วาง 500g อ่าน raw=50000 → factor=100.0f
 */
void HX711_SetCalibration(HX711_Instance* hx, float factor) {
    if (hx == NULL || !hx->initialized) return;
    if (factor == 0.0f) return;  /* ป้องกัน division by zero */
    hx->calibration_factor = factor;
}

/**
 * @brief อ่านน้ำหนักในหน่วย gram
 * @param hx     - instance (NULL หรือไม่ initialized → return HX711_ERROR_PARAM)
 * @param weight - pointer รับน้ำหนัก (gram), (NULL → return HX711_ERROR_PARAM)
 * @return HX711_OK หรือ error จาก HX711_Read()
 * @note  weight = (raw - tare_offset) / calibration_factor
 *        เรียก HX711_Read() ภายใน 1 ครั้ง
 *        calibration_factor = 0 จะทำให้ division by zero
 */
HX711_Status HX711_GetWeight(HX711_Instance* hx, float* weight) {
    if (hx == NULL || !hx->initialized || weight == NULL) return HX711_ERROR_PARAM;

    int32_t raw;
    HX711_Status st = HX711_Read(hx, &raw);
    if (st != HX711_OK) return st;

    *weight = (float)(raw - hx->tare_offset) / hx->calibration_factor;
    return HX711_OK;
}

/**
 * @brief อ่านน้ำหนักเฉลี่ยหลายครั้ง
 * @param hx      - instance (NULL หรือไม่ initialized → return HX711_ERROR_PARAM)
 * @param samples - จำนวน samples (1-HX711_MAX_SAMPLES, 0/เกิน → ใช้ 5)
 * @param weight  - pointer รับน้ำหนักเฉลี่ย (gram), (NULL → return HX711_ERROR_PARAM)
 * @return HX711_OK หรือ HX711_ERROR_PARAM / HX711_ERROR_TIMEOUT
 * @note  samples=0 หรือเกิน HX711_MAX_SAMPLES จะถูกปรับเป็น 5
 *        ค่าเฉลี่ย raw ใช้ int64_t ก่อนหาร
 *        weight = (avg_raw - tare_offset) / calibration_factor
 */
HX711_Status HX711_GetWeightAvg(HX711_Instance* hx, uint8_t samples, float* weight) {
    if (hx == NULL || !hx->initialized || weight == NULL) return HX711_ERROR_PARAM;
    if (samples == 0 || samples > HX711_MAX_SAMPLES) samples = 5;

    int64_t sum = 0;
    for (uint8_t i = 0; i < samples; i++) {
        if (!_wait_ready(hx)) return HX711_ERROR_TIMEOUT;
        sum += _read_raw(hx);
    }

    int32_t avg_raw = (int32_t)(sum / samples);
    *weight = (float)(avg_raw - hx->tare_offset) / hx->calibration_factor;
    return HX711_OK;
}

/**
 * @brief เปลี่ยน Gain / Channel
 * @param hx   - instance (NULL หรือไม่ initialized → return HX711_ERROR_PARAM)
 * @param gain - HX711_GAIN_128, HX711_GAIN_64, หรือ HX711_GAIN_32
 * @return HX711_OK หรือ HX711_ERROR_TIMEOUT
 * @note  gain pulse count = enum value (1=Gain128, 2=Gain32, 3=Gain64)
 *        ต้องอ่าน dummy 1 ครั้งเพื่อ apply gain ใหม่
 *        Gain ใหม่จะมีผลกับ pulse count ในการอ่านครั้งถัดไป
 */
HX711_Status HX711_SetGain(HX711_Instance* hx, HX711_Gain gain) {
    if (hx == NULL || !hx->initialized) return HX711_ERROR_PARAM;

    hx->gain = gain;

    /* อ่าน 1 ครั้งเพื่อ apply gain ใหม่ */
    if (!_wait_ready(hx)) return HX711_ERROR_TIMEOUT;
    _read_raw(hx);

    return HX711_OK;
}

/**
 * @brief Power Down HX711 (ประหยัดไฟ)
 * @param hx - instance (NULL หรือไม่ initialized → return HX711_ERROR_PARAM)
 * @return HX711_OK
 * @note  ตั้ง SCK=HIGH ค้าง 80µs (>60µs ตาม datasheet)
 *        กระแสไฟฟ้าลดลง < 1µA ในโหมด power down
 *        ไม่ต้องรอ DOUT ก่อน
 */
HX711_Status HX711_PowerDown(HX711_Instance* hx) {
    if (hx == NULL || !hx->initialized) return HX711_ERROR_PARAM;

    /* SCK HIGH > 60µs → HX711 เข้า power down */
    digitalWrite(hx->pin_sck, 1);
    Delay_Us(80);

    return HX711_OK;
}

/**
 * @brief Power Up HX711
 * @param hx - instance (NULL หรือไม่ initialized → return HX711_ERROR_PARAM)
 * @return HX711_OK หรือ HX711_ERROR_TIMEOUT
 * @note  ตั้ง SCK=LOW เพื่อออกจาก power down
 *        รอ HX711 เริ่มต้น 500ms (≈10Hz output rate)
 *        อ่าน dummy 1 ครั้งเพื่อ apply gain
 *        หลัง power up DOUT จะ LOW เมื่อพร้อม
 */
HX711_Status HX711_PowerUp(HX711_Instance* hx) {
    if (hx == NULL || !hx->initialized) return HX711_ERROR_PARAM;

    /* SCK LOW → HX711 ออกจาก power down */
    digitalWrite(hx->pin_sck, 0);

    /* รอ HX711 reset (ราว 400ms ที่ 10Hz output rate) */
    Delay_Ms(500);

    /* อ่าน 1 ครั้งเพื่อ apply gain */
    if (!_wait_ready(hx)) return HX711_ERROR_TIMEOUT;
    _read_raw(hx);

    return HX711_OK;
}

/**
 * @brief แปลง HX711_Status เป็น string
 * @param st - ค่า HX711_Status
 * @return string constant เช่น "OK", "ERROR_PARAM", "ERROR_TIMEOUT", "UNKNOWN"
 * @note  ถ้า st ไม่ตรงกับ enum → return "UNKNOWN"
 */
const char* HX711_StatusStr(HX711_Status st) {
    switch (st) {
        case HX711_OK:            return "OK";
        case HX711_ERROR_PARAM:   return "ERROR_PARAM";
        case HX711_ERROR_TIMEOUT: return "ERROR_TIMEOUT";
        default:                  return "UNKNOWN";
    }
}
