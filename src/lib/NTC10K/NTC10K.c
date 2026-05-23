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
 * @param ntc Pointer to NTC instance with divider configuration
 * @param adc_value Raw ADC reading (0 - ADC_MAX_VALUE)
 * @return Calculated NTC resistance in ohms, or NAN if ADC value is at rail
 * @note  Supports two divider configurations: NTC-to-GND and NTC-to-VCC.
 *        Uses voltage divider formula with R_series.
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
 * @param ntc Pointer to NTC instance with B-value and nominal resistance/temperature
 * @param resistance Measured NTC resistance in ohms
 * @return Temperature in °C, or NAN if resistance is invalid
 * @note  Beta equation: 1/T = 1/T0 + (1/B) × ln(R/R0).
 *        T0 is the nominal temperature in Kelvin.
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
 * @param ntc Pointer to NTC instance with Steinhart-Hart coefficients A, B, C
 * @param resistance Measured NTC resistance in ohms
 * @return Temperature in °C, or NAN if resistance is invalid
 * @note  Steinhart-Hart equation: 1/T = A + B×ln(R) + C×(ln(R))³.
 *        More accurate than Beta equation over a wide temperature range.
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
 * @brief ใช้ค่า calibration (offset + gain) กับอุณหภูมิ
 * @param ntc Pointer to NTC instance with calibration parameters
 * @param temperature Raw calculated temperature in °C
 * @return Calibrated temperature = temperature × gain + offset
 */
static float ApplyCalibration(NTC_Instance* ntc, float temperature) {
    if (isnan(temperature)) {
        return NAN;
    }
    
    return (temperature * ntc->calibration_gain) + ntc->calibration_offset;
}

/**
 * @brief ตรวจสอบขีดจำกัดและเรียก callback
 * @param ntc Pointer to NTC instance with threshold and callback configuration
 * @param temperature Current temperature to evaluate against thresholds
 * @note  Calls on_threshold_high if temperature exceeds threshold_high,
 *        calls on_threshold_low if temperature drops below threshold_low.
 *        Only fires if threshold_enabled is true.
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
 * @param adc_channel ADC channel connected to the NTC voltage divider
 * @return Pointer to initialized NTC_Instance, or NULL if instance pool is full
 * @note  Uses default configuration: Beta method, 10k series resistor, 25°C nominal,
 *        3950 B-value, NTC-to-GND divider. For custom config use NTC_InitWithConfig().
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
 * @param config Pointer to NTC_Config structure with all parameters
 * @return Pointer to initialized NTC_Instance, or NULL if instance pool is full
 * @note  Copies the config into a static instance slot. Initializes the ADC channel(s).
 *        Defaults: calibration_offset=0, calibration_gain=1, thresholds disabled.
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
 * @param ntc Pointer to initialized NTC_Instance
 * @return Temperature in °C, or NAN on failure (invalid instance, ADC error)
 * @note  Reads ADC, calculates resistance, applies Beta or Steinhart-Hart equation,
 *        then applies calibration offset/gain. Caches result in last_temperature.
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
 * @param ntc Pointer to initialized NTC_Instance
 * @return Temperature in °F, or NAN on failure
 * @note  Converts Celsius result from NTC_ReadTemperature() using °C × 9/5 + 32.
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
 * @param ntc Pointer to initialized NTC_Instance
 * @return NTC resistance in ohms, or NAN on failure
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
 * @param ntc Pointer to initialized NTC_Instance
 * @return Raw ADC value (0 - ADC_MAX_VALUE), or 0 if not initialized
 */
uint16_t NTC_ReadADC(NTC_Instance* ntc) {
    if (ntc == NULL || !ntc->initialized) {
        return 0;
    }
    
    return ADC_Read(ntc->config.adc_channel);
}

/**
 * @brief อ่านอุณหภูมิแบบ average หลายครั้ง
 * @param ntc Pointer to initialized NTC_Instance
 * @param samples Number of samples to average (must be > 0)
 * @return Averaged temperature in °C, or NAN if no valid samples
 * @note  Reads temperature multiple times with 100 µs delay between samples.
 *        Invalid readings (NAN) are excluded from the average.
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
 * @param ntc Pointer to initialized NTC_Instance
 * @param offset Temperature offset in °C
 */
void NTC_SetCalibration(NTC_Instance* ntc, float offset) {
    if (ntc != NULL && ntc->initialized) {
        ntc->calibration_offset = offset;
    }
}

/**
 * @brief ตั้งค่า calibration แบบ offset และ gain
 * @param ntc Pointer to initialized NTC_Instance
 * @param offset Temperature offset in °C
 * @param gain Scaling factor (default 1.0)
 */
void NTC_SetCalibrationAdvanced(NTC_Instance* ntc, float offset, float gain) {
    if (ntc != NULL && ntc->initialized) {
        ntc->calibration_offset = offset;
        ntc->calibration_gain = gain;
    }
}

/**
 * @brief ตั้งค่า B-value สำหรับ Beta equation
 * @param ntc Pointer to initialized NTC_Instance
 * @param b_value Beta coefficient (typically 3000-5000 for NTC thermistors)
 */
void NTC_SetBValue(NTC_Instance* ntc, float b_value) {
    if (ntc != NULL && ntc->initialized) {
        ntc->config.b_value = b_value;
    }
}

/**
 * @brief ตั้งค่า Steinhart-Hart coefficients
 * @param ntc Pointer to initialized NTC_Instance
 * @param a Steinhart-Hart A coefficient
 * @param b Steinhart-Hart B coefficient
 * @param c Steinhart-Hart C coefficient
 */
void NTC_SetCoefficients(NTC_Instance* ntc, float a, float b, float c) {
    if (ntc != NULL && ntc->initialized) {
        ntc->config.sh_a = a;
        ntc->config.sh_b = b;
        ntc->config.sh_c = c;
    }
}

/**
 * @brief เปลี่ยนวิธีการคำนวณ (Beta หรือ Steinhart-Hart)
 * @param ntc Pointer to initialized NTC_Instance
 * @param method NTC_METHOD_BETA or NTC_METHOD_STEINHART_HART
 */
void NTC_SetMethod(NTC_Instance* ntc, NTC_CalculationMethod method) {
    if (ntc != NULL && ntc->initialized) {
        ntc->config.method = method;
    }
}

/**
 * @brief ตั้งค่าขีดจำกัดอุณหภูมิ
 * @param ntc Pointer to initialized NTC_Instance
 * @param low Lower temperature threshold (°C)
 * @param high Upper temperature threshold (°C)
 */
void NTC_SetThreshold(NTC_Instance* ntc, float low, float high) {
    if (ntc != NULL && ntc->initialized) {
        ntc->threshold_low = low;
        ntc->threshold_high = high;
    }
}

/**
 * @brief ตั้งค่า callback functions สำหรับ threshold events
 * @param ntc Pointer to initialized NTC_Instance
 * @param on_high Callback invoked when temperature exceeds high threshold (receives °C)
 * @param on_low Callback invoked when temperature falls below low threshold (receives °C)
 */
void NTC_SetCallback(NTC_Instance* ntc, void (*on_high)(float), void (*on_low)(float)) {
    if (ntc != NULL && ntc->initialized) {
        ntc->on_threshold_high = on_high;
        ntc->on_threshold_low = on_low;
    }
}

/**
 * @brief เปิด/ปิดการตรวจสอบขีดจำกัด
 * @param ntc Pointer to initialized NTC_Instance
 * @param enabled true to enable threshold monitoring, false to disable
 */
void NTC_EnableThreshold(NTC_Instance* ntc, bool enabled) {
    if (ntc != NULL && ntc->initialized) {
        ntc->threshold_enabled = enabled;
    }
}

/**
 * @brief อัพเดทสถานะของ NTC (อ่านอุณหภูมิและตรวจสอบ thresholds)
 * @param ntc Pointer to initialized NTC_Instance
 * @note  Reads temperature and fires callbacks if thresholds are exceeded.
 *        Call this periodically from the main loop.
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
 * @param celsius Temperature in degrees Celsius
 * @return Equivalent temperature in degrees Fahrenheit
 */
float NTC_CelsiusToFahrenheit(float celsius) {
    return (celsius * 9.0f / 5.0f) + 32.0f;
}

/**
 * @brief แปลง °F เป็น °C
 * @param fahrenheit Temperature in degrees Fahrenheit
 * @return Equivalent temperature in degrees Celsius
 */
float NTC_FahrenheitToCelsius(float fahrenheit) {
    return (fahrenheit - 32.0f) * 5.0f / 9.0f;
}

/**
 * @brief ตรวจสอบว่าค่าอุณหภูมิที่อ่านได้ถูกต้อง
 * @param ntc Pointer to initialized NTC_Instance
 * @param temperature Temperature value to validate
 * @return true if temperature is not NAN and within [NTC_TEMP_MIN, NTC_TEMP_MAX]
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
 * @note  Calls NTC_Update() on every initialized instance in the static pool.
 *        Useful for polling all sensors in the main loop.
 */
void NTC_UpdateAll(void) {
    for (uint8_t i = 0; i < ntc_instance_count; i++) {
        NTC_Update(&ntc_instances[i]);
    }
}

/**
 * @brief หา NTC instance จาก ADC channel
 * @param adc_channel ADC channel to search for
 * @return Pointer to NTC_Instance using the given channel, or NULL if not found
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
