/********************************** (C) COPYRIGHT ********************************
 * File Name          : main.c
 * Author             : SimpleHAL
 * Version            : V1.1
 * Date               : 2026/05/17
 * Description        : ตัวอย่างที่ 11 — BLE peripheral
 *
 * สิ่งที่เรียนรู้:
 * - การ init BLE stack (BLE_LibInit + GAP Role)
 * - การตั้งชื่ออุปกรณ์ (device name)
 * - การเริ่ม advertising ให้อุปกรณ์อื่นค้นพบ
 * - การแนบ callback (connect, disconnect, data)
 * - การเรียก hal_ble_process() เพื่อประมวลผล BLE stack
 *
 * ข้อกำหนด:
 * - ต้องมี BLE library ของ WCH (.a/.o) ใน project
 * - ต้องมี include path ไปยัง BLE/LIB
 * - ต้องเพิ่ม interrupt handlers: BLEL_IRQHandler
 *
 * การทำงาน:
 * 1. init BLE → เริ่ม advertising
 * 2. รอให้ smartphone/app เชื่อมต่อ
 * 3. เมื่อเชื่อมต่อ → LED ติด
 * 4. เมื่อได้รับข้อมูล → LED กลับค่า
 * 5. เมื่อขาดการเชื่อมต่อ → LED ดับ
 *******************************************************************************/

#include "simple_hal.h"

/*
 * ตัวแปร global สำหรับเก็บ handle (จำเป็นเพราะสงไปยัง callback)
 */
static hal_ble_handle_t ble;    // BLE handle
static hal_gpio_handle_t led;   // GPIO handle

/*
 * ฟังก์ชัน callback — เมื่อมีอุปกรณ์เชื่อมต่อ
 *
 * ถูกเรียกโดย BLE stack เมื่อ Central device (เช่น smartphone)
 * เชื่อมต่อกับอุปกรณ์นี้สำเร็จ
 *
 * พารามิเตอร์:
 * - arg: พอยน์เตอร์จาก hal_ble_attach_connect_cb() (NULL ในตัวอย่าง)
 */
void on_connect(void *arg)
{
    (void)arg;

    /*
     * เมื่อเชื่อมต่อ → สั่งให้ LED ติด
     * เพื่อแสดงว่าอยู่ในสถานะ connected
     *
     * หมายเหตุ: callback นี้ทำงานในบริบทของ BLE event
     * ไม่ควรใช้ hal_delay_ms() หรือ blocking operation
     */
    hal_gpio_write(led, 1);
}

/*
 * ฟังก์ชัน callback — เมื่อขาดการเชื่อมต่อ
 *
 * ถูกเรียกเมื่อ:
 * - Central device ถอดการเชื่อมต่อ
 * - สัญญาณขาดหาย (out of range)
 * - เกิด error ในการเชื่อมต่อ
 */
void on_disconnect(void *arg)
{
    (void)arg;

    /*
     * เมื่อขาดการเชื่อมต่อ → สั่งให้ LED ดับ
     * อุปกรณ์จะกลับไป advertising เพื่อรอการเชื่อมต่อใหม่
     */
    hal_gpio_write(led, 0);
}

/*
 * ฟังก์ชัน callback — เมื่อได้รับข้อมูล
 *
 * ถูกเรียกเมื่อ Central device ส่งข้อมูล (notification)
 * มายังอุปกรณ์นี้
 *
 * BLE notification: สามารถส่งข้อมูลได้สูงสุด 20 ไบต์
 * ต่อ 1 notification
 */
void on_data(void *arg)
{
    (void)arg;

    /*
     * กลับค่า LED เมื่อได้รับข้อมูล
     * เพื่อแสดงว่ามีข้อมูลเข้ามา
     *
     * โดยปกติแล้วควรอ่านข้อมูลที่ได้รับด้วย
     * callback เพิ่มเติมหรือ GATT read
     */
    hal_gpio_toggle(led);
}

/*
 * จุดเริ่มต้นโปรแกรม
 */
int main()
{
    /* ตั้งค่านาฬิกา — BLE ต้องการความถี่สูง (100MHz) เพื่อความแม่นยำ */
    HSECFG_Capacitance(HSECap_18p);
    SetSysClock(CLK_SOURCE_HSE_PLL_100MHz);

    /* เริ่มต้น GPIO PA11 สำหรับ LED บอกสถานะ */
    led = hal_gpio_init(PA11, HAL_GPIO_OUTPUT_PP_5mA);

    /*
     * เริ่มต้น BLE peripheral
     *
     * hal_ble_init(device_name, tx_power)
     * - "nanoCH572": ชื่ออุปกรณ์ที่จะแสดงใน BLE scan
     *   (สูงสุด 15 ตัวอักษร)
     * - LL_TX_POWEER_0_DBM: กำลังส่ง 0 dBm (~1 mW)
     *   ค่ามาตรฐาน: 0x12 (0 dBm)
     *   ค่าอื่น: LL_TX_POWEER_4_DBM, LL_TX_POWEER_NEG12_DBM ฯลฯ
     *
     * การทำงานภายใน hal_ble_init(): (ทำอัตโนมัติ)
     * 1. ตรวจสอบเวอร์ชัน header ตรงกับ library หรือไม่
     * 2. ตั้งค่า SysTick, MISC_CTRL สำหรับ BLE library
     * 3. ตั้งค่า LLE IRQ handler location
     * 4. กำหนดค่า PFIC priority สำหรับ BLE interrupt
     * 5. เรียก BLE_LibInit() เพื่อเริ่ม BLE stack
     * 6. ลงทะเบียน TMOS task
     * 7. เริ่ม GAP Peripheral Role
     * 8. ตั้งค่า advertising data (ชื่ออุปกรณ์)
     */
    ble = hal_ble_init("nanoCH572", LL_TX_PWR_0_DBM);

    /*
     * แนบ callback function
     *
     * hal_ble_attach_connect_cb(ble, on_connect, NULL)
     * - กำหนดฟังก์ชันที่เรียกเมื่อมีอุปกรณ์เชื่อมต่อ
     *
     * hal_ble_attach_disconnect_cb(ble, on_disconnect, NULL)
     * - กำหนดฟังก์ชันที่เรียกเมื่อขาดการเชื่อมต่อ
     *
     * hal_ble_attach_data_cb(ble, on_data, NULL)
     * - กำหนดฟังก์ชันที่เรียกเมื่อได้รับข้อมูล
     */
    hal_ble_attach_connect_cb(ble, on_connect, NULL);
    hal_ble_attach_disconnect_cb(ble, on_disconnect, NULL);
    hal_ble_attach_data_cb(ble, on_data, NULL);

    /*
     * เริ่ม advertising — ให้อุปกรณ์อื่นค้นพบ
     *
     * hal_ble_advertise_start(handle, mode)
     * - mode: HAL_BLE_ADV_MODE_CONNECTABLE
     *   = แบบให้เชื่อมต่อได้ (ทั่วไป)
     *
     * เมื่อเริ่ม advertising แล้ว smartphone ที่support BLE
     * จะเห็นอุปกรณ์ "nanoCH572" ในรายการ scan
     */
    hal_ble_advertise_start(ble, HAL_BLE_ADV_MODE_CONNECTABLE);

    /*
     * main loop
     *
     * hal_ble_process() — สำคัญมาก!
     * ฟังก์ชันนี้เรียก TMOS_SystemProcess() ซึ่งจัดการ event
     * ทั้งหมดของ BLE stack
     *
     * ต้องเรียก hal_ble_process() เป็นระยะ (อย่างน้อยทุก ~10ms)
     * เพื่อให้ BLE stack ทำงาน:
     * - ประมวลผล event จาก radio
     * - จัดการ advertising
     * - จัดการ connection
     * - ส่ง/รับข้อมูล
     *
     * without hal_ble_process(), BLE stack จะไม่ทำงาน!
     *
     * hal_delay_ms(10): หน่วง 10ms แต่ละรอบ
     * ถ้าหน่วงนานเกินไป BLE จะตอบสนองช้า
     * ถ้าหน่วงน้อยไป CPU จะทำงานหนัก
     * 10ms เป็นค่าสมดุลที่ดี
     */
    while (1) {
        hal_ble_process();
        hal_delay_ms(10);
    }

    /*
     * 流程:
     * 1. init → LED = ดับ
     * 2. advertising → รอเชื่อมต่อ
     * 3. connected → LED = ติด
     * 4. receive data → LED = กลับค่า
     * 5. disconnected → LED = ดับ → กลับไป step 2
     */
}
