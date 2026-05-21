/********************************** (C) COPYRIGHT ********************************
 * File Name          : main.c
 * Author             : SimpleHAL
 * Version            : V1.1
 * Date               : 2026/05/17
 * Description        : ตัวอย่างที่ 13 — รวมทุกฟีเจอร์สาธิต SimpleHAL
 *
 * สิ่งที่เรียนรู้:
 * - การใช้งานหลายโมดูลพร้อมกันในโปรแกรมเดียว
 * - การใช้ Timer interrupt กับ UART async ทำงานร่วมกัน
 * - การใช้ hal_delay_ms() ใน main loop กับ event-driven callback
 * - โครงสร้างโปรแกรม MCU ที่ซับซ้อนขึ้น
 *
 * การทำงาน:
 * - Timer ทุก 1 วินาที → กลับค่า LED + เพิ่ม sys_ticks
 * - UART echo (เมื่อมีข้อมูลเข้า → ส่งกลับ)
 * - ทุก 5 วินาที: อ่าน ADC + แสดง UID (ถ้ามี UART)
 * - main loop: หน่วง 10ms เพื่อให้ UART rx callback ทำงาน
 *******************************************************************************/

#include "simple_hal.h"

/*
 * ตัวแปร static ระดับโมดูล
 *
 * uart: handle UART สำหรับรับ/ส่งข้อมูล
 * led: handle GPIO สำหรับ LED
 * tmr: handle Timer สำหรับ interrupt ทุก 1 วินาที
 * sys_ticks: ตัวนับจำนวนครั้งที่ Timer interrupt เกิดขึ้น
 */
static hal_uart_handle_t uart;
static hal_gpio_handle_t led;
static hal_timer_handle_t tmr;
static uint32_t sys_ticks = 0;  // เริ่มนับจาก 0

/*
 * Timer callback — ทำงานทุก 1 วินาที
 *
 * โปรดสังเกตว่า hal_delay_ms ห้ามใช้ใน callback!
 * แต่การเพิ่มตัวนับ (sys_ticks++) ปลอดภัยเพราะใช้เวลาไม่กี่คำสั่ง
 */
void timer_cb(void *arg)
{
    (void)arg;

    /*
     * sys_ticks++: เพิ่มค่าตัวนับ
     * ใช้ volatile หรือ atomic ก็ดี แต่สำหรับตัวอย่างนี้ static ก็พอ
     * เนื่องจาก main loop อ่านค่าแค่ทุก 10ms แถม uint32_t ก็เป็น atomic
     * บน RISC-V 32-bit อยู่แล้ว
     */
    sys_ticks++;

    /*
     * กลับค่า LED ทุก 1 วินาที
     * ทำให้ LED กระพริบเป็นจังหวะสม่ำเสมอ
     */
    hal_gpio_toggle(led);
}

/*
 * UART RX callback — เมื่อมีข้อมูลขาเข้า
 *
 * ทำงานทุกครั้งที่ UART ได้รับข้อมูล
 * (มาจาก UART RX interrupt → ring buffer → callback)
 *
 * เป็นระบบ echo: อ่านข้อมูลแล้วส่งกลับทันที
 * (ใช้ ring buffer ดังนั้นรับได้แม้ callback ยังไม่ถูกเรียก)
 */
void uart_rx_cb(void *arg)
{
    /*
     * แคสต์ arg → hal_uart_handle_t
     * arg ถูกส่งมาจาก hal_uart_attach_rx_cb() ใน main
     */
    hal_uart_handle_t h = (hal_uart_handle_t)arg;

    uint8_t buf[64];
    uint16_t len;

    /*
     * อ่านข้อมูลจาก ring buffer
     * hal_uart_recv() แบบ non-blocking (คืน HAL_BUSY ถ้าไม่มีข้อมูล)
     * แต่ใน callback ที่ถูกเรียกเพราะมีข้อมูล ก็ควรมีข้อมูลเสมอ
     */
    if (hal_uart_recv(h, buf, sizeof(buf), &len) == HAL_OK) {

        /*
         * ส่งข้อมูลกลับ (echo)
         * การเรียก hal_uart_send() ใน callback ปกติไม่ควรทำ
         * เพราะ hal_uart_send() เป็น blocking
         *
         * แต่ในตัวอย่างนี้ยอมรับได้เพราะ UART speed 115200
         * การส่ง ~64 ไบต์ใช้เวลาไม่ถึง 6ms
         *
         * วิธีที่ดีกว่า: ใช้ hal_uart_send_async() แทน
         */
        hal_uart_send(h, buf, len);
    }
}

/*
 * จุดเริ่มต้นโปรแกรม
 */
int main()
{
    uint16_t adc_val;    // ค่า raw จาก ADC
    uint32_t mv;         // ค่าแรงดัน (mV) จาก ADC
    uint8_t uid[8];      // Unique ID 8 ไบต์

    /* ตั้งค่านาฬิกา */
    HSECFG_Capacitance(HSECap_18p);
    SetSysClock(CLK_SOURCE_HSE_PLL_100MHz);

    /*
     * เริ่มต้นทุกโมดูล
     *
     * ลำดับ: เริ่มจากพื้นฐานก่อน → แล้วค่อยเริ่มที่ซับซ้อน
     * GPIO → UART → Timer → ADC → Flash
     */

    /* 1. GPIO — PA11 สำหรับ LED */
    led = hal_gpio_init(PA11, HAL_GPIO_OUTPUT_PP_5mA);

    /* 2. UART — 115200 baud สำหรับ debug/echo */
    uart = hal_uart_init(115200, UART_DEFAULT_TX_PIN, UART_DEFAULT_RX_PIN);

    /* 3. Timer — 1,000,000 µs (1 วินาที) แบบ periodic */
    tmr = hal_timer_init(1000000, HAL_TIMER_MODE_PERIODIC);

    /*
     * 4. ADC — 9 บิต ที่ขา PA2
     *    เริ่มต้นหลังจาก GPIO/UART/Timer เพราะต้องการ Timer
     *    สำหรับวัดค่า (ADC ใช้ CMP + Timer)
     */
    hal_adc_handle_t adc = hal_adc_init(HAL_ADC_9BIT, PA2);

    /*
     * 5. Flash — อ่าน UID (ข้อมูลจากโรงงาน ไม่ต้อง init)
     */
    hal_flash_get_unique_id(uid);

    /*
     * แนบ Timer callback และเริ่ม Timer
     *
     * หลังจากนี้ timer_cb() จะถูกเรียกทุก 1 วินาที
     * ซึ่งจะไปเพิ่ม sys_ticks และ toggle LED
     */
    hal_timer_attach_cb(tmr, timer_cb, NULL);
    hal_timer_start(tmr);

    /*
     * แนบ UART RX callback
     *
     * หลังจากนี้ uart_rx_cb() จะถูกเรียกทุกครั้ง
     * ที่ UART ได้รับข้อมูล (echo)
     */
    hal_uart_attach_rx_cb(uart, uart_rx_cb, uart);

    /*
     * main loop
     *
     * โครงสร้าง:
     * - หน่วง 10ms เพื่อให้ CPU ไม่ทำงานหนัก
     * - ตรวจสอบ sys_ticks ทุก 10ms
     * - ถ้า sys_ticks >= 5 (ผ่านไป 5 วินาที):
     *   อ่าน ADC, เตรียม UART (ไม่ได้ส่งจริงในตัวอย่างนี้)
     *
     * รอบเวลาสูงสุดของ Timer interrupt:
     *   1 วินาที × 5 = 5 วินาที ระหว่างการแสดงผล
     *
     * หมายเหตุ: hal_delay_ms(10) = 10ms
     * ดังนั้นใน 1 วินาที (1000ms) จะวน loop 100 รอบ
     * sys_ticks จะเพิ่มขึ้น 1 ทุกวินาที (จาก Timer interrupt)
     * เราตรวจสอบทุก 10ms → ตอบสนองภายใน 10ms
     */
    while (1) {

        /*
         * ตรวจสอบ sys_ticks ทุกครั้งที่วน loop
         *
         * ถ้า sys_ticks >= 5 แปลว่าผ่านไป 5 วินาที
         * (เพราะ timer interrupt เกิดขึ้นทุก 1 วินาที)
         */
        if (sys_ticks >= 5) {

            /*
             * รีเซ็ต sys_ticks = 0 เพื่อเริ่มนับ 5 วินาทีใหม่
             * ต้องรีเซ็ตก่อน ADC read เพราะ ADC ใช้ Timer
             * แต่ในทางปฏิบัติ ADC กับ Timer ใช้คนละ Timer
             *
             * คำเตือน: ในระบบจริงควรใช้ atomic operation
             * หรือ disable interrupt ขณะปรับค่า sys_ticks
             */
            sys_ticks = 0;

            /*
             * อ่านค่า ADC
             * - adc_val: ค่า raw (0-511 สำหรับ 9 บิต)
             * - mv: แรงดันที่คำนวณ (mV) โดยใช้ VREF = 3.3V
             *
             * ในตัวอย่างนี้ไม่ได้ส่งค่าเหล่านี้ผ่าน UART
             * (เพื่อให้โปรแกรมกระชับ)
             * แต่คุณสามารถเพิ่ม hal_uart_send() ได้
             */
            adc_val = hal_adc_read(adc);
            mv = hal_adc_read_mv(adc, 3300);

            /*
             * หมายเหตุ: uid[] มี Unique ID 8 ไบต์
             * อ่านตั้งแต่เริ่มโปรแกรม (ก่อน while loop)
             * พร้อมใช้งานได้ตลอด
             *
             * ตัวอย่างการส่ง UID ผ่าน UART:
             *   hal_uart_send(uart, uid, 8);
             */
        }

        /*
         * หน่วงเวลา 10ms
         *
         * ค่านี้สำคัญ:
         * - ถ้าน้อยไป: CPU ทำงานหนัก (เปลืองไฟ)
         * - ถ้ามากไป: การตอบสนองต่อเหตุการณ์ช้า
         * 10ms = balance ที่ดีสำหรับตัวอย่างนี้
         *
         * โดยปกติแล้วควรใช้ __WFI() แทน hal_delay_ms()
         * แต่ hal_delay_ms สะดวกกว่าในตัวอย่างรวมหลายโมดูล
         */
        hal_delay_ms(10);
    }

    /*
     * สรุปการทำงานทั้งหมดในตัวอย่างนี้:
     *
     * ┌─────────────────────────────────────────────────┐
     * │ Timer (1s)                                      │
     * │   → timer_cb: sys_ticks++, toggle LED           │
     * │                                                 │
     * │ UART RX                                         │
     * │   → uart_rx_cb: echo ข้อมูลกลับ                  │
     * │                                                 │
     * │ Main loop (ทุก 10ms)                            │
     * │   → ถ้า sys_ticks >= 5:                         │
     * │       อ่าน ADC, reset sys_ticks                 │
     * └─────────────────────────────────────────────────┘
     */
}
