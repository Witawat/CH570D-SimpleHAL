/**
 * @file NTC10K.c
 * @brief NTC 10K Thermistor Temperature Sensor Library Implementation
 * @version 1.0
 * @date 2025-12-22
 */

#include "NTC10K.h"

/* ========== Private Variables ========== */

static NTC_Instance ntc_instances[NTC_MAX_INSTANCES];
static uint8_t ntc_instance_count = 0;

/* ========== Private Helper Functions ========== */

/**
 * @brief คำนวณความต้านทานของ NTC จาก ADC value
 */
static float CalculateResistance(NTC_Instance* ntc, uint16_t adc_value) {
    if (adc_value == 0 || adc_value >= ADC_MAX_VALUE) {
        return NAN;  // Invalid reading
    }
    
    float resistance;
    
    if (ntc->config.divider_config == NTC_DIVIDER_NTC_TO_GND) {
        // NTC ต่อลง GND: R_NTC = R_series × (ADC / (1023 - ADC))
        resistance = ntc->config.r_series * ((float)adc_value / (ADC_MAX_VALUE - (float)adc_value));
    } else {
        // NTC ต่อขึ้น VCC: R_NTC = R_series × ((1023 - ADC) / ADC)
        resistance = ntc->config.r_series * ((ADC_MAX_VALUE - (float)adc_value) / (float)adc_value);
    }
    
    return resistance;
}

/**
 * @brief คำนวณอุณหภูมิด้วย Beta equation
 */
static float CalculateTempBeta(NTC_Instance* ntc, float resistance) {
    if (isnan(resistance) || resistance <= 0) {
        return NAN;
    }
    
    // Beta equation: 1/T = 1/T0 + (1/B) × ln(R/R0)
    // T = 1 / (1/T0 + (1/B) × ln(R/R0))
    
    float t0_kelvin = ntc->config.t_nominal + 273.15f;  // แปลง °C เป็น K
    float ln_r_ratio = logf(resistance / ntc->config.r_nominal);
    float temp_kelvin = 1.0f / ((1.0f / t0_kelvin) + (ln_r_ratio / ntc->config.b_value));
    float temp_celsius = temp_kelvin - 273.15f;
    
    return temp_celsius;
}

/**
 * @brief คำนวณอุณหภูมิด้วย Steinhart-Hart equation
 */
static float CalculateTempSteinhartHart(NTC_Instance* ntc, float resistance) {
    if (isnan(resistance) || resistance <= 0) {
        return NAN;
    }
    
    // Steinhart-Hart equation: 1/T = A + B×ln(R) + C×(ln(R))³
    
    float ln_r = logf(resistance);
    float ln_r_cubed = ln_r * ln_r * ln_r;
    
    float temp_kelvin_inv = ntc->config.sh_a + 
                           (ntc->config.sh_b * ln_r) + 
                           (ntc->config.sh_c * ln_r_cubed);
    
    float temp_kelvin = 1.0f / temp_kelvin_inv;
    float temp_celsius = temp_kelvin - 273.15f;
    
    return temp_celsius;
}

/**
 * @brief ใช้ค่า calibration
 */
static float ApplyCalibration(NTC_Instance* ntc, float temperature) {
    if (isnan(temperature)) {
        return NAN;
    }
    
    return (temperature * ntc->calibration_gain) + ntc->calibration_offset;
}

/**
 * @brief ตรวจสอบขีดจำกัดและเรียก callback
 */
static void CheckThresholds(NTC_Instance* ntc, float temperature) {
    if (!ntc->threshold_enabled || isnan(temperature)) {
        return;
    }
    
    // ตรวจสอบ threshold สูง
    if (temperature > ntc->threshold_high && ntc->on_threshold_high != NULL) {
        ntc->on_threshold_high(temperature);
    }
    
    // ตรวจสอบ threshold ต่ำ
    if (temperature < ntc->threshold_low && ntc->on_threshold_low != NULL) {
        ntc->on_threshold_low(temperature);
    }
}

/* ========== Public Functions ========== */

/**
 * @brief เริ่มต้นการใช้งาน NTC sensor แบบง่าย
 */
NTC_Instance* NTC_Init(ADC_Channel adc_channel) {
    NTC_Config config = {
        .adc_channel = adc_channel,
        .r_series = NTC_DEFAULT_R_SERIES,
        .r_nominal = NTC_DEFAULT_R_NOMINAL,
        .t_nominal = NTC_DEFAULT_T_NOMINAL,
        .b_value = NTC_DEFAULT_B_VALUE,
        .vcc = NTC_DEFAULT_VCC,
        .method = NTC_METHOD_BETA,
        .divider_config = NTC_DIVIDER_NTC_TO_GND,
        .sh_a = NTC_SH_A,
        .sh_b = NTC_SH_B,
        .sh_c = NTC_SH_C
    };
    
    return NTC_InitWithConfig(&config);
}

/**
 * @brief เริ่มต้น NTC พร้อม configuration
 */
NTC_Instance* NTC_InitWithConfig(NTC_Config* config) {
    if (ntc_instance_count >= NTC_MAX_INSTANCES) {
        return NULL;  // เต็มแล้ว
    }
    
    // หา instance ว่าง
    NTC_Instance* ntc = &ntc_instances[ntc_instance_count++];
    
    // คัดลอก configuration
    ntc->config = *config;
    
    // ตั้งค่า calibration เริ่มต้น
    ntc->calibration_offset = 0.0f;
    ntc->calibration_gain = 1.0f;
    
    // ตั้งค่า threshold เริ่มต้น
    ntc->threshold_high = NTC_TEMP_MAX;
    ntc->threshold_low = NTC_TEMP_MIN;
    ntc->threshold_enabled = false;
    
    // ตั้งค่า callbacks
    ntc->on_threshold_high = NULL;
    ntc->on_threshold_low = NULL;
    
    // ตั้งค่า state
    ntc->last_temperature = NAN;
    ntc->initialized = true;
    
    // เริ่มต้น ADC channel
    ADC_Channel channels[] = {config->adc_channel};
    ADC_SimpleInitChannels(channels, 1);
    
    return ntc;
}

/**
 * @brief อ่านอุณหภูมิ (°C)
 */
float NTC_ReadTemperature(NTC_Instance* ntc) {
    if (ntc == NULL || !ntc->initialized) {
        return NAN;
    }
    
    // อ่านค่า ADC
    uint16_t adc_value = ADC_Read(ntc->config.adc_channel);
    
    // คำนวณความต้านทาน
    float resistance = CalculateResistance(ntc, adc_value);
    if (isnan(resistance)) {
        return NAN;
    }
    
    // คำนวณอุณหภูมิตามวิธีที่เลือก
    float temperature;
    if (ntc->config.method == NTC_METHOD_STEINHART_HART) {
        temperature = CalculateTempSteinhartHart(ntc, resistance);
    } else {
        temperature = CalculateTempBeta(ntc, resistance);
    }
    
    // ใช้ calibration
    temperature = ApplyCalibration(ntc, temperature);
    
    // บันทึกค่าล่าสุด
    ntc->last_temperature = temperature;
    
    return temperature;
}

/**
 * @brief อ่านอุณหภูมิ (°F)
 */
float NTC_ReadTemperatureF(NTC_Instance* ntc) {
    float temp_c = NTC_ReadTemperature(ntc);
    if (isnan(temp_c)) {
        return NAN;
    }
    return NTC_CelsiusToFahrenheit(temp_c);
}

/**
 * @brief อ่านค่าความต้านทานของ NTC
 */
float NTC_ReadResistance(NTC_Instance* ntc) {
    if (ntc == NULL || !ntc->initialized) {
        return NAN;
    }
    
    uint16_t adc_value = ADC_Read(ntc->config.adc_channel);
    return CalculateResistance(ntc, adc_value);
}

/**
 * @brief อ่านค่า ADC raw
 */
uint16_t NTC_ReadADC(NTC_Instance* ntc) {
    if (ntc == NULL || !ntc->initialized) {
        return 0;
    }
    
    return ADC_Read(ntc->config.adc_channel);
}

/**
 * @brief อ่านอุณหภูมิแบบ average หลายครั้ง
 */
float NTC_ReadAverage(NTC_Instance* ntc, uint8_t samples) {
    if (ntc == NULL || !ntc->initialized || samples == 0) {
        return NAN;
    }
    
    float sum = 0.0f;
    uint8_t valid_samples = 0;
    
    for (uint8_t i = 0; i < samples; i++) {
        float temp = NTC_ReadTemperature(ntc);
        if (!isnan(temp)) {
            sum += temp;
            valid_samples++;
        }
        Delay_Us(100);  // หน่วงเวลาเล็กน้อยระหว่างการอ่าน
    }
    
    if (valid_samples == 0) {
        return NAN;
    }
    
    return sum / valid_samples;
}

/**
 * @brief ตั้งค่า calibration offset
 */
void NTC_SetCalibration(NTC_Instance* ntc, float offset) {
    if (ntc != NULL && ntc->initialized) {
        ntc->calibration_offset = offset;
    }
}

/**
 * @brief ตั้งค่า calibration แบบ offset และ gain
 */
void NTC_SetCalibrationAdvanced(NTC_Instance* ntc, float offset, float gain) {
    if (ntc != NULL && ntc->initialized) {
        ntc->calibration_offset = offset;
        ntc->calibration_gain = gain;
    }
}

/**
 * @brief ตั้งค่า B-value
 */
void NTC_SetBValue(NTC_Instance* ntc, float b_value) {
    if (ntc != NULL && ntc->initialized) {
        ntc->config.b_value = b_value;
    }
}

/**
 * @brief ตั้งค่า Steinhart-Hart coefficients
 */
void NTC_SetCoefficients(NTC_Instance* ntc, float a, float b, float c) {
    if (ntc != NULL && ntc->initialized) {
        ntc->config.sh_a = a;
        ntc->config.sh_b = b;
        ntc->config.sh_c = c;
    }
}

/**
 * @brief เปลี่ยนวิธีการคำนวณ
 */
void NTC_SetMethod(NTC_Instance* ntc, NTC_CalculationMethod method) {
    if (ntc != NULL && ntc->initialized) {
        ntc->config.method = method;
    }
}

/**
 * @brief ตั้งค่าขีดจำกัดอุณหภูมิ
 */
void NTC_SetThreshold(NTC_Instance* ntc, float low, float high) {
    if (ntc != NULL && ntc->initialized) {
        ntc->threshold_low = low;
        ntc->threshold_high = high;
    }
}

/**
 * @brief ตั้งค่า callback functions
 */
void NTC_SetCallback(NTC_Instance* ntc, void (*on_high)(float), void (*on_low)(float)) {
    if (ntc != NULL && ntc->initialized) {
        ntc->on_threshold_high = on_high;
        ntc->on_threshold_low = on_low;
    }
}

/**
 * @brief เปิด/ปิดการตรวจสอบขีดจำกัด
 */
void NTC_EnableThreshold(NTC_Instance* ntc, bool enabled) {
    if (ntc != NULL && ntc->initialized) {
        ntc->threshold_enabled = enabled;
    }
}

/**
 * @brief อัพเดทสถานะของ NTC
 */
void NTC_Update(NTC_Instance* ntc) {
    if (ntc == NULL || !ntc->initialized) {
        return;
    }
    
    float temperature = NTC_ReadTemperature(ntc);
    CheckThresholds(ntc, temperature);
}

/**
 * @brief แปลง °C เป็น °F
 */
float NTC_CelsiusToFahrenheit(float celsius) {
    return (celsius * 9.0f / 5.0f) + 32.0f;
}

/**
 * @brief แปลง °F เป็น °C
 */
float NTC_FahrenheitToCelsius(float fahrenheit) {
    return (fahrenheit - 32.0f) * 5.0f / 9.0f;
}

/**
 * @brief ตรวจสอบว่าค่าอุณหภูมิที่อ่านได้ถูกต้อง
 */
bool NTC_IsValid(NTC_Instance* ntc, float temperature) {
    if (ntc == NULL || !ntc->initialized) {
        return false;
    }
    
    if (isnan(temperature)) {
        return false;
    }
    
    // ตรวจสอบว่าอยู่ในช่วงที่เป็นไปได้
    if (temperature < NTC_TEMP_MIN || temperature > NTC_TEMP_MAX) {
        return false;
    }
    
    return true;
}

/**
 * @brief อัพเดท NTC instances ทั้งหมด
 */
void NTC_UpdateAll(void) {
    for (uint8_t i = 0; i < ntc_instance_count; i++) {
        NTC_Update(&ntc_instances[i]);
    }
}

/**
 * @brief หา NTC instance จาก ADC channel
 */
NTC_Instance* NTC_GetInstanceByChannel(ADC_Channel adc_channel) {
    for (uint8_t i = 0; i < ntc_instance_count; i++) {
        if (ntc_instances[i].config.adc_channel == adc_channel && 
            ntc_instances[i].initialized) {
            return &ntc_instances[i];
        }
    }
    return NULL;
}
