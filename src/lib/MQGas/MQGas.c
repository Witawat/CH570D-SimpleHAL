/**
 * @file MQGas.c
 * @brief MQ Gas Sensor Library Implementation
 * @version 1.0
 * @date 2026-04-29
 */

#include "MQGas.h"
#include <math.h>

/* ========== Private: Curve parameter table ========== */

typedef struct {
    float a;
    float b;
    float ro_clean_air_ratio; /**< Rs/Ro ใน clean air (จาก datasheet) */
} _MQCurve;

static const _MQCurve _curve_table[] = {
    /* MQ_TYPE_GENERIC */ { 100.0f, -1.5f,  1.0f  },
    /* MQ_TYPE_MQ2     */ { 574.25f, -2.222f, 9.83f },
    /* MQ_TYPE_MQ3     */ { 0.3934f, -1.504f, 60.0f },
    /* MQ_TYPE_MQ4     */ { 1012.7f, -2.786f, 4.4f  },
    /* MQ_TYPE_MQ5     */ { 1163.8f, -3.874f, 6.5f  },
    /* MQ_TYPE_MQ6     */ { 2127.2f, -2.526f, 10.0f },
    /* MQ_TYPE_MQ7     */ { 99.042f, -1.518f, 27.5f },
    /* MQ_TYPE_MQ9     */ { 1000.5f, -2.186f, 9.6f  },
    /* MQ_TYPE_MQ135   */ { 110.47f, -2.862f, 3.6f  },
};

/* ========== Private: Calculate Rs from ADC ========== */

static float _calc_rs(MQGas_Instance* mq) {
    uint16_t raw = ADC_Read(mq->adc_ch);

    /* แรงดัน AOUT (ปรับตาม voltage divider ถ้ามี) */
    float vout = ADC_ToVoltage(raw, mq->vref);
    if (vout <= 0.0f) vout = 0.001f;  /* ป้องกัน division by zero */

    /* Rs = RL × (Vcc - Vout) / Vout */
    float rs = mq->rl * (mq->vcc - vout) / vout;
    if (rs <= 0.0f) rs = 0.001f;
    return rs;
}

/* ========== Public Functions ========== */

MQGas_Status MQGas_Init(MQGas_Instance* mq, ADC_Channel ch,
                          MQGas_Type type, float vcc, float vref) {
    if (mq == NULL) return MQGAS_ERROR_PARAM;
    if (vcc <= 0.0f || vref <= 0.0f) return MQGAS_ERROR_PARAM;

    mq->adc_ch       = ch;
    mq->type         = type;
    mq->vcc          = vcc;
    mq->vref         = vref;
    mq->rl           = MQGAS_RL_KOHM;
    mq->ro           = 10.0f;  /* default, ต้อง calibrate */
    mq->threshold    = 500.0f; /* default threshold 500 PPM */
    mq->is_calibrated = 0;
    mq->initialized  = 0;

    /* ตั้งค่า curve จาก table */
    uint8_t idx = (uint8_t)type;
    if (idx >= (sizeof(_curve_table) / sizeof(_curve_table[0])))
        idx = 0;

    mq->curve_a = _curve_table[idx].a;
    mq->curve_b = _curve_table[idx].b;

    mq->initialized = 1;
    return MQGAS_OK;
}

void MQGas_SetCurve(MQGas_Instance* mq, float a, float b) {
    if (mq == NULL || !mq->initialized) return;
    if (a <= 0.0f) return;
    mq->curve_a = a;
    mq->curve_b = b;
}

MQGas_Status MQGas_Calibrate(MQGas_Instance* mq, uint8_t samples) {
    if (mq == NULL || !mq->initialized) return MQGAS_ERROR_PARAM;
    if (samples == 0 || samples > MQGAS_MAX_CALIB_SAMPLES) samples = 50;

    float sum = 0.0f;
    for (uint8_t i = 0; i < samples; i++) {
        sum += _calc_rs(mq);
        Delay_Ms(50);
    }
    float rs_clean = sum / (float)samples;

    /* Ro = Rs_clean / Rs_Ro_ratio (จาก datasheet) */
    uint8_t idx = (uint8_t)mq->type;
    if (idx >= (sizeof(_curve_table) / sizeof(_curve_table[0]))) idx = 0;
    float ratio = _curve_table[idx].ro_clean_air_ratio;
    if (ratio <= 0.0f) ratio = 1.0f;

    mq->ro = rs_clean / ratio;
    mq->is_calibrated = 1;
    return MQGAS_OK;
}

void MQGas_SetRo(MQGas_Instance* mq, float ro) {
    if (mq == NULL || !mq->initialized) return;
    if (ro <= 0.0f) return;
    mq->ro = ro;
    mq->is_calibrated = 1;
}

uint16_t MQGas_ReadRaw(MQGas_Instance* mq) {
    if (mq == NULL || !mq->initialized) return 0;
    return ADC_Read(mq->adc_ch);
}

float MQGas_ReadVoltage(MQGas_Instance* mq) {
    if (mq == NULL || !mq->initialized) return 0.0f;
    return ADC_ToVoltage(ADC_Read(mq->adc_ch), mq->vref);
}

float MQGas_GetRs(MQGas_Instance* mq) {
    if (mq == NULL || !mq->initialized) return 0.0f;
    return _calc_rs(mq);
}

float MQGas_GetPPM(MQGas_Instance* mq) {
    if (mq == NULL || !mq->initialized) return -1.0f;
    if (!mq->is_calibrated) return -1.0f;

    float rs = _calc_rs(mq);
    float ratio = rs / mq->ro;

    /* PPM = A × (Rs/Ro)^B */
    return mq->curve_a * powf(ratio, mq->curve_b);
}

void MQGas_SetThreshold(MQGas_Instance* mq, float threshold) {
    if (mq == NULL || !mq->initialized) return;
    mq->threshold = threshold;
}

uint8_t MQGas_IsAlarm(MQGas_Instance* mq) {
    if (mq == NULL || !mq->initialized || !mq->is_calibrated) return 0;
    float ppm = MQGas_GetPPM(mq);
    return (ppm >= mq->threshold) ? 1 : 0;
}
