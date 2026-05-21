/********************************** (C) COPYRIGHT ********************************
 * File Name          : main.c
 * Author             : SimpleHAL
 * Version            : V1.1
 * Date               : 2026/05/17
 * Description        : ตัวอย่างที่ 6 — อ่านค่า ADC (ผ่าน CMP emulation)
 *
 * สิ่งที่เรียนรู้:
 * - การ init ADC (ใช้ Comparator + Timer เพื่ออ่านค่าแอนะล็อก)
 * - การอ่านค่า raw ADC
 * - การแปลงค่า raw เป็นแรงดันไฟฟ้า (mV)
 *
 * หมายเหตุสำคัญ:
 * CH572 ไม่มี ADC โดยตรง — SimpleHAL ใช้ CMP (Comparator) +
 * Timer + resistor/capacitor เพื่อจำลองการอ่านค่าแอนะล็อก
 * ความละเอียด: 4, 8, หรือ 9 บิต
 *
 * การต่อวงจร:
 * - ต่อสัญญาณแอนะล็อก (0-3.3V) เข้าที่ PA2
 * - ต่อ capacitor 100nF ระหว่าง PA2 กับ GND เพื่อลด noise
 *******************************************************************************/

#include "simple_hal.h"

int main()
{
    /*
     * adc_val — ค่า raw ที่อ่านได้จาก ADC
     *   - 9-bit: 0-511
     *   - 8-bit: 0-255
     *   - 4-bit: 0-15
     *
     * mv — แรงดันไฟฟ้าที่คำนวณจาก adc_val
     * แปลงเป็นมิลลิโวลต์ (mV) เพื่อให้อ่านง่าย
     */
    uint16_t adc_val;
    uint32_t mv;

    /* ตั้งค่านาฬิกา */
    HSECFG_Capacitance(HSECap_18p);
    SetSysClock(CLK_SOURCE_HSE_PLL_100MHz);

    /*
     * เริ่มต้น ADC
     *
     * hal_adc_init(resolution, pin)
     * - resolution: HAL_ADC_9BIT — ความละเอียด 9 บิต (ค่า 0-511)
     *   ค่าที่เป็นไปได้: HAL_ADC_4BIT, HAL_ADC_8BIT, HAL_ADC_9BIT
     *   ยิ่งบิตมาก ยิ่งละเอียด แต่ใช้เวลาอ่านนานขึ้น
     * - pin: PA2 — ขาที่ใช้วัดสัญญาณแอนะล็อก
     *
     * การทำงาน: SimpleHAL ใช้ CMP (Comparator) ภายใน CH572
     * โดยเปรียบเทียบแรงดันที่ขากับค่า internal reference
     * แล้วใช้ Timer วัดเวลาที่ capacitor charge จนถึงระดับนั้น
     */
    hal_adc_handle_t adc = hal_adc_init(HAL_ADC_9BIT, PA2);

    /*
     * main loop: อ่านค่า ADC ทุก 500ms
     */
    while (1) {

        /*
         * hal_adc_read(handle) — อ่านค่า raw ADC
         * คืนค่าเป็น uint16_t (แต่ค่าที่ได้ขึ้นกับความละเอียด)
         *   - 9-bit: 0-511 (512 ระดับ)
         *   - แต่ละระดับ = 3300/512 ≈ 6.45mV
         *
         * ฟังก์ชันนี้เป็นแบบ blocking รอจนกว่าจะวัดเสร็จ
         */
        adc_val = hal_adc_read(adc);

        /*
         * hal_adc_read_mv(handle, vref_mv) — แปลงค่า ADC เป็น mV
         * - vref_mv: 3300 (3.3V) — แรงดันอ้างอิง
         *
         * การคำนวณ:
         *   mV = (adc_val * vref_mv) / (2^resolution)
         *   เช่น 9-bit: mV = (adc_val * 3300) / 512
         *
         * ถ้า adc_val = 256 → mV = (256 * 3300) / 512 = 1650 mV = 1.65V
         */
        mv = hal_adc_read_mv(adc, 3300);

        /*
         * รอ 500ms ก่อนอ่านครั้งถัดไป
         * ในโปรแกรมจริงควรแสดงค่าผ่าน UART
         */
        hal_delay_ms(500);
    }
}
