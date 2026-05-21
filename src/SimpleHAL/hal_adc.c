#include "hal_adc.h"
#include "simple_hal_config.h"
#include "CH57x_common.h"

/** @brief โครงสร้างข้อมูลภายในของ ADC */
struct hal_adc_obj {
    uint8_t  used;        /** @brief flag ว่าถูกใช้งานแล้ว */
    uint8_t  resolution;  /** @brief ความละเอียด 4/8/9 บิต */
    uint8_t  pin;         /** @brief ขาที่ใช้วัด */
};

static struct hal_adc_obj adc_instances[HAL_ADC_MAX_INSTANCES];

/*********************************************************************
 * @fn      adc_4bit_sample
 *
 * @brief   สุ่มค่า ADC แบบ 4 บิต ใช้ comparator + SAR
 *
 * @return  ค่า ADC 4 บิต
 */
static uint32_t adc_4bit_sample(void)
{
    uint32_t vref = 8;
    uint32_t cmp_count = 1;
    while (1) {
        uint8_t cmp_out = 0;
        uint8_t cmp_out_last = 0;
        CMP_Init(cmp_sw_3, vref - 1);
        CMP_Enable();
        DelayUs(1);
        cmp_out_last = CMP_ReadAPROut();
        while (1) {
            cmp_out = CMP_ReadAPROut();
            if (cmp_out == cmp_out_last) break;
            cmp_out_last = cmp_out;
        }
        if (cmp_count == 8) {
            if (cmp_out) {
                if (vref == 15) vref = 16;
                else return vref * 2 + 1;
            } else {
                return vref * 2 - 1;
            }
        } else {
            cmp_count <<= 1;
            if (cmp_out) vref += 8 / cmp_count;
            else vref -= 8 / cmp_count;
        }
    }
}

/*********************************************************************
 * @fn      adc_8bit_sample
 *
 * @brief   สุ่มค่า ADC แบบ 8 บิต ใช้ comparator + PWM สร้างแรงดันอ้างอิง
 *
 * @return  ค่า ADC 8 บิต
 */
static uint32_t adc_8bit_sample(void)
{
    uint32_t vref = 128;
    uint32_t cmp_count = 1;
    GPIOA_ModeCfg(GPIO_Pin_4, GPIO_ModeOut_PP_5mA);
    PWM_16bit_CycleDisable();
    PWMX_CLKCfg(1);
    PWMX_CycleCfg(PWMX_Cycle_256);
    CMP_Init(cmp_sw_2, cmp_nref_level_800);
    CMP_Enable();
    while (1) {
        uint8_t cmp_out = 0;
        uint8_t cmp_out_last = 0;
        PWMX_ACTOUT(CH_PWM4, vref, High_Level, ENABLE);
        DelayUs(760);
        cmp_out_last = CMP_ReadAPROut();
        while (1) {
            DelayUs(20);
            cmp_out = CMP_ReadAPROut();
            if (cmp_out == cmp_out_last) break;
            cmp_out_last = cmp_out;
        }
        if (cmp_count == 128) {
            if (cmp_out) return vref * 2 - 1;
            else return vref * 2 + 1;
        } else {
            cmp_count <<= 1;
            if (cmp_out) vref -= 128 / cmp_count;
            else vref += 128 / cmp_count;
        }
    }
}

/*********************************************************************
 * @fn      adc_9bit_sample
 *
 * @brief   สุ่มค่า ADC แบบ 9 บิต ใช้ comparator + PWM 16-bit
 *
 * @return  ค่า ADC 9 บิต
 */
static uint32_t adc_9bit_sample(void)
{
    uint32_t vref = 256;
    uint32_t cmp_count = 1;
    GPIOA_ModeCfg(GPIO_Pin_4, GPIO_ModeOut_PP_5mA);
    PWMX_CLKCfg(1);
    PWM_16bit_CycleEnable();
    PWMX_16bit_CycleCfg(CH_PWM4, 512);
    CMP_Init(cmp_sw_2, cmp_nref_level_800);
    CMP_Enable();
    while (1) {
        uint8_t cmp_out = 0;
        uint8_t cmp_out_last = 0;
        PWMX_16bit_ACTOUT(CH_PWM4, vref, High_Level, ENABLE);
        DelayUs(230);
        cmp_out_last = CMP_ReadAPROut();
        while (1) {
            DelayUs(20);
            cmp_out = CMP_ReadAPROut();
            if (cmp_out == cmp_out_last) break;
            cmp_out_last = cmp_out;
        }
        if (cmp_count == 256) {
            if (cmp_out) return vref * 2 - 1;
            else return vref * 2 + 1;
        } else {
            cmp_count <<= 1;
            if (cmp_out) vref -= 256 / cmp_count;
            else vref += 256 / cmp_count;
        }
    }
}

/*********************************************************************
 * @fn      hal_adc_init
 *
 * @brief   เริ่มต้น ADC: บันทึกความละเอียดและขา
 *
 * @param   res - ความละเอียด ADC
 * @param   pin - ขาที่ใช้วัด
 *
 * @return  handle ของ ADC
 */
hal_adc_handle_t hal_adc_init(hal_adc_resolution_t res, uint8_t pin)
{
    (void)pin;
    hal_adc_handle_t h = &adc_instances[0];
    if (h->used) return h;
    h->used = 1;
    h->resolution = (uint8_t)res;
    h->pin = pin;
    return h;
}

/*********************************************************************
 * @fn      hal_adc_read
 *
 * @brief   อ่านค่า ADC ดิบตามความละเอียดที่ตั้งไว้
 *
 * @param   h - handle ของ ADC
 *
 * @return  ค่า ADC ดิบ
 */
uint16_t hal_adc_read(hal_adc_handle_t h)
{
    if (!h || !h->used) return 0;
    switch (h->resolution) {
    case HAL_ADC_4BIT: return (uint16_t)adc_4bit_sample();
    case HAL_ADC_8BIT: return (uint16_t)adc_8bit_sample();
    case HAL_ADC_9BIT: return (uint16_t)adc_9bit_sample();
    default: return 0;
    }
}

/*********************************************************************
 * @fn      hal_adc_read_mv
 *
 * @brief   อ่านค่า ADC และแปลงเป็นแรงดัน (mV)
 *
 * @param   h - handle ของ ADC
 * @param   vref_mv - แรงดันอ้างอิง (mV)
 *
 * @return  แรงดันไฟฟ้า (mV)
 */
uint32_t hal_adc_read_mv(hal_adc_handle_t h, uint16_t vref_mv)
{
    if (!h || !h->used) return 0;
    uint16_t val = hal_adc_read(h);
    switch (h->resolution) {
    case HAL_ADC_4BIT: return (uint32_t)val * 25;
    case HAL_ADC_8BIT: return (uint32_t)val * vref_mv / 512;
    case HAL_ADC_9BIT: return (uint32_t)val * vref_mv / 1024;
    default: return 0;
    }
}

/*********************************************************************
 * @fn      hal_adc_free
 *
 * @brief   คืนทรัพยากร ADC
 *
 * @param   h - handle ของ ADC
 */
void hal_adc_free(hal_adc_handle_t h)
{
    if (h) h->used = 0;
}
