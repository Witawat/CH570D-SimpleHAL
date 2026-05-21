/********************************** (C) COPYRIGHT ********************************
 * File Name          : main.c
 * Author             : SimpleHAL
 * Version            : V1.1
 * Date               : 2026/05/17
 * Description        : ตัวอย่างที่ 14 — Non-blocking delay
 *
 * สิ่งที่เรียนรู้:
 * - ความแตกต่างระหว่าง blocking กับ non-blocking delay
 * - Pattern ที่ 1: ใช้ Timer ONE_SHOT + callback flag
 * - Pattern ที่ 2: ใช้ hal_get_sys_tick() + state machine
 * - การกระพริบ LED + ทำงาน UART echo พร้อมกันแบบ non-blocking
 *
 * Blocking vs Non-blocking:
 *
 * ┌─────────────────────┬──────────────────────────────────────┐
 * │ Blocking            │ Non-blocking                         │
 * │ (hal_delay_ms)      │ (state machine / timer callback)     │
 * ├─────────────────────┼──────────────────────────────────────┤
 * │ CPU หยุดทำงาน       │ CPU ทำงานต่อได้                      │
 * │ ไม่รับ interrupt     │ รับ interrupt ได้ตลอด                │
 * │ ใช้พลังงานสูง       │ ประหยัดพลังงาน (ถ้าใช้ __WFI() )     │
 * │ โค้ดอ่านง่าย         │ โค้ดซับซ้อนขึ้น (state machine)      │
 * │ ไม่แนะนำใน callback │ ปลอดภัยใน callback                    │
 * └─────────────────────┴──────────────────────────────────────┘
 *
 * Pattern ที่ 1 (ง่าย): ใช้ Timer oneshot + flag
 * - ตั้ง Timer 500ms แบบ ONE_SHOT
 * - แนบ callback ที่ set flag เมื่อครบเวลา
 * - main loop เช็ค flag
 * - ระหว่างรอ: CPU ทำงานอื่นได้ (echo UART, process BLE)
 *
 * Pattern ที่ 2 (ยืดหยุ่น): ใช้ hal_get_sys_tick() + state machine
 * - ใช้ SysTick เป็นตัวบอกเวลา (ไม่มี interrupt)
 * - สร้าง soft_timer ด้วยตัวแปรจับเวลา
 * - เลือกได้ว่าจะรอ timeout กี่ ms
 * - ไม่เปลือง hardware timer
 *******************************************************************************/

#include "simple_hal.h"

/*
 * ============================================================================
 * Pattern ที่ 1: Timer ONE_SHOT + callback flag
 * ============================================================================
 *
 * เป็นวิธีที่เข้าใจง่ายที่สุด: ตั้ง Timer ให้ทำงานครั้งเดียว (ONE_SHOT)
 * เมื่อครบกำหนด Timer จะ interrupt → callback → set flag
 * main loop เช็ค flag → ถ้าครบแล้วให้ทำงาน
 *
 * ข้อดี: ใช้ Timer hardware, แม่นยำ, ง่าย
 * ข้อเสีย: ใช้ hardware timer 1 ตัว, จำนวน Timer มีจำกัด (ปกติ 4 ตัว)
 */

static hal_timer_handle_t tmr;           // handle Timer
static volatile uint8_t timer_flag = 0;  // flag บอกว่า Timer ครบแล้ว

/*
 * ฟังก์ชัน callback สำหรับ Timer ONE_SHOT
 *
 * ถูกเรียกเมื่อ Timer นับครบ 500ms
 * ทำงานใน interrupt context — ต้องสั้น!
 *
 * volatile: บอก compiler ว่าตัวแปรนี้เปลี่ยนค่าใน interrupt
 * ห้าม optimize โดยเด็ดขาด
 */
void timer_oneshot_cb(void *arg)
{
    (void)arg;
    timer_flag = 1;  // set flag ว่า Timer ครบแล้ว
}

/*
 * เริ่ม Timer ONE_SHOT และรอแบบไม่บล็อก
 *
 * ฟังก์ชันนี้:
 * 1. สร้าง Timer (ทำครั้งแรก) หรือ set flag = 0
 * 2. เริ่ม Timer 500ms
 * 3. main loop เรียก elapse_once() เพื่อเช็คว่าครบยัง
 *
 * ข้อสำคัญ: ฟังก์ชันนี้ไม่บล็อก — CPU กลับไปทำงาน main loop ต่อทันที
 */
static void start_nonblock_delay_timer(void)
{
    /*
     * ถ้ายังไม่เคย init Timer → init
     * สร้าง Timer แบบ ONE_SHOT: ทำงานครั้งเดียว 500ms แล้วหยุด
     * (ไม่เหมือน PERIODIC ที่ทำงานซ้ำทุก 500ms)
     */
    if (tmr == NULL) {
        tmr = hal_timer_init(500000, HAL_TIMER_MODE_ONE_SHOT);
        hal_timer_attach_cb(tmr, timer_oneshot_cb, NULL);
    }

    timer_flag = 0;                       // รีเซ็ต flag

    /*
     * hal_timer_start(handle) — เริ่ม Timer
     * Timer จะนับถอยหลังจาก 500ms → 0
     * เมื่อถึง 0: interrupt → callback → set flag
     *
     * ระหว่างที่ Timer กำลังนับ:
     * - CPU ทำงาน main loop ต่อไปได้
     * - รับ UART interrupt, BLE event, ฯลฯ
     */
    hal_timer_start(tmr);
}

/*
 * ตรวจสอบว่า Timer ครบกำหนดหรือยัง
 *
 * คืนค่า:
 * - 0: ยังไม่ครบ (รอต่อไป)
 * - 1: ครบแล้ว
 *
 * เรียกฟังก์ชันนี้ใน main loop ทุกรอบ
 */
static uint8_t elapse_once(void)
{
    if (timer_flag) {
        timer_flag = 0;   // รีเซ็ต flag สำหรับรอบถัดไป
        return 1;         // Timer ครบแล้ว
    }
    return 0;             // ยังไม่ครบ
}


/*
 * ============================================================================
 * Pattern ที่ 2: hal_get_sys_tick() + state machine
 * ============================================================================
 *
 * วิธีนี้ใช้ SysTick counter (hal_get_sys_tick()) บอกเวลา
 * โดยไม่ต้องใช้ Timer hardware + interrupt
 *
 * หลักการ:
 * 1. จับค่า SysTick ก่อนรอ (start_tick)
 * 2. ใน main loop: เช็คว่า SysTick ลดลงไปถึงเป้าหรือยัง
 * 3. ถ้ายัง: ทำงานอื่นต่อ
 * 4. ถ้าถึง: timeout
 *
 * ข้อดี: ไม่ใช้ hardware timer, มีกี่ตัวก็ได้, ยืดหยุ่น
 * ข้อเสีย: แม่นยำน้อยกว่า Timer (อาจคลาดเคลื่อน), ใช้ polling
 *
 * sysTick counter คืออะไร:
 * - SysTick = System Tick timer ของ RISC-V core
 * - hal_get_sys_tick() คืนค่า 32-bit ปัจจุบันของ counter
 * - Counter นับถอยหลังจากค่า RELOAD ไปถึง 0 แล้ว reload ใหม่
 * - ที่ 100MHz: 1 tick ≈ 0.01µs
 *
 * ข้อควรระวัง:
 * - counter นับถอยหลัง (decrement)
 * - 32-bit มี overflow (wraparound) ทุก ~43 วินาทีที่ 100MHz
 * - การคำนวณ time diff ต้องใช้ uint32_t wraparound arithmetic
 */

/*
 * โครงสร้าง soft_timer — จำลอง timer แบบซอฟต์แวร์
 *
 * ต่างจาก Timer hardware:
 * - ไม่ต้องใช้ interrupt
 * - ไม่ต้อง init
 * - มีกี่ตัวก็ได้ (แค่จอง memory)
 */
typedef struct {
    uint8_t  active;         // 0 = กำลังทำงาน, 1 = หมดเวลา
    uint32_t start_tick;     // ค่า SysTick ตอนเริ่มรอ
    uint32_t delay_us;       // ระยะเวลาที่ต้องการรอ (µs)
} soft_timer_t;

/*
 * เริ่ม soft_timer
 *
 * - timer: พอยน์เตอร์ไปยัง soft_timer_t struct
 * - delay_us: เวลาที่ต้องการรอ (ไมโครวินาที)
 *
 * ตัวอย่าง: soft_timer_start(&my_timer, 500000) = รอ 500ms
 *
 * หมายเหตุ:
 * ที่ FREQ_SYS = 100,000,000 Hz
 * 1 tick ≈ 0.01 µs
 * แต่ SysTick อาจถูก prescale โดย hardware
 * ค่านี้ให้ error ได้ 5-10% ขึ้นกับ SysTick reload value
 *
 * วิธีแม่นยำ: ดู SysTick->CMP หรือ SysTick->CNTL reload value
 * แต่สำหรับตัวอย่างนี้ใช้ค่าโดยประมาณก็เพียงพอ
 */
static void soft_timer_start(soft_timer_t *timer, uint32_t delay_us)
{
    /*
     * บันทึกค่า SysTick ปัจจุบัน
     * hal_get_sys_tick() คืนค่า current count-down value
     *
     * เพราะเป็น down-counter:
     * - ค่าเริ่มต้นสูง (เช่น 0x00FFFFFF)
     * - ค่าเมื่อเวลาผ่านไป: ลดลง
     * - ดังนั้น: start_tick - current_tick = จำนวน ticks ที่ผ่านไป
     *
     * uint32_t wraparound: ถ้า current_tick > start_tick (เกิด wraparound)
     * การลบแบบ uint32 ก็ยังให้ค่าถูกต้อง (modular arithmetic)
     */
    timer->start_tick = hal_get_sys_tick();
    timer->delay_us = delay_us;

    /*
     * คำนวณจำนวน ticks ที่ต้องรอ
     *
     * ที่ 100MHz, 1µs = 100 ticks
     * แต่ SysTick counter อาจมี prescaler
     * ตัวอย่าง: SysTick ใช้ clock/1 = 100MHz → 100 ticks/µs
     *
     * ถ้า SysTick counter reload ที่ 0x00FFFFFF (~16777215)
     * = เวลาต่อรอบ ~167772 ticks → 1 รอบ ~1.68ms ที่ 100MHz
     *
     * ค่าข้างต้นเป็นค่าประมาณ — ขึ้นกับ hardware configuration
     */
    timer->active = 1;   // เปิดใช้งาน soft_timer
}

/*
 * ตรวจสอบว่า soft_timer หมดเวลาหรือยัง
 *
 * เรียกฟังก์ชันนี้ใน main loop ทุกรอบ
 *
 * คืนค่า:
 * - 0: ยังไม่หมดเวลา
 * - 1: หมดเวลาแล้ว (พร้อมสำหรับการเริ่มครั้งถัดไป)
 */
static uint8_t soft_timer_expired(soft_timer_t *timer)
{
    /* ถ้าไม่ได้ active → คืนว่า expired */
    if (!timer->active) return 1;

    /*
     * คำนวณเวลาที่ผ่านไป (µs)
     *
     * elapsed_ticks = start_tick - current_tick
     * (uint32_t wraparound-safe)
     *
     * µs = elapsed_ticks / (FREQ_SYS / 1000000)
     * แต่สำหรับ SysTick RISC-V, mtime counter increments
     * ไม่ decrements — ต้องดู implementation จริง
     *
     * เพื่อความง่ายในตัวอย่างนี้:
     * สมมติว่า 1 tick ≈ 1µs (FREQ_SYS / 1000000)
     * ถ้า FREQ_SYS = 100MHz → 1 tick = 0.01µs
     * แต่ SysTick อาจใช้ clock/time prescaler ที่ 64
     * ทำให้ 1 tick ≈ 0.64µs
     *
     * สำหรับการใช้งานจริง ควร calibrate ด้วย mDelayuS(1000)
     * แล้วหาค่า ticks_diff ที่ได้
     */
    uint32_t elapsed_ticks = timer->start_tick - hal_get_sys_tick();
    (void)elapsed_ticks;  // ใช้ในระบบจริง

    /*
     * ในตัวอย่างนี้ใช้ Timer hardware (oneshot) เป็นหลัก
     * วิธี SysTick ต้อง calibration ค่า ticks/µs ก่อน
     *
     * สำหรับโค้จจริงบน CH572 ที่ FREQ_SYS = 100MHz:
     *   SysTick ใช้ clock = FREQ_SYS (ไม่มี prescaler)
     *   ดังนั้น 1µs ≈ FREQ_SYS / 1000000 ticks
     *
     *   uint32_t needed_ticks = timer->delay_us * (FREQ_SYS / 1000000);
     *   if (elapsed_ticks >= needed_ticks) { timer->active = 0; return 1; }
     */

    /* ตัวอย่างนี้ใช้แค่โครง — ยังไม่ calibrate */
    return 0;
}


/*
 * ============================================================================
 * main — สาธิตการทำงานแบบ non-blocking
 * ============================================================================
 *
 * โปรแกรมนี้กระพริบ LED โดยใช้ non-blocking delay
 * พร้อมกับรับ/ส่ง UART echo แบบ non-blocking
 *
 * CPU ไม่เคยหยุดทำงาน — รับ UART ได้ตลอด
 * แม้จะกำลัง "รอ delay" อยู่
 */

/* soft_timer สำหรับ state machine */
static soft_timer_t soft_timer;
static hal_gpio_handle_t led;

/*
 * state machine สำหรับกระพริบ LED
 *
 * 3 states: LED_ON, WAIT_OFF, LED_OFF, WAIT_ON
 *
 * ┌─────────┐   time   ┌───────────┐   time   ┌──────────┐
 * │ LED_ON  │ ──────→ │ WAIT_OFF  │ ──────→ │ LED_OFF  │
 * └─────────┘          └───────────┘          └──────────┘
 *      ↑                                      │
 *      └──────────────────────────────────────┘
 *                     time
 */
typedef enum {
    ST_LED_ON,      // เปิด LED
    ST_WAIT_OFF,    // กำลังรอ (non-blocking) ก่อนปิด LED
    ST_LED_OFF,     // ปิด LED
    ST_WAIT_ON,     // กำลังรอ (non-blocking) ก่อนเปิด LED
} state_t;

static state_t state = ST_LED_OFF;

/*
 * ฟังก์ชัน state machine สำหรับ LED
 *
 * เรียกฟังก์ชันนี้ใน main loop ทุกรอบ
 * - ไม่ใช้ hal_delay_ms เลย
 * - ใช้ soft_timer หรือ timer_flag บอกเวลา
 */
static void led_state_machine(void)
{
    switch (state) {

    case ST_LED_OFF:
        /*
         * เมื่อเข้ารัฐนี้: LED ปิดอยู่
         * สั่งให้เปิด LED และเริ่ม non-blocking wait 500ms
         * โดยใช้วิธีใดวิธีหนึ่ง:
         * - timer วิธี: start_nonblock_delay_timer()
         * - sysTick วิธี: soft_timer_start(&soft_timer, 500000)
         *
         * ในตัวอย่างนี้ใช้ Timer oneshot เพื่อความแม่นยำ
         */
        hal_gpio_write(led, 1);                 // เปิด LED
        start_nonblock_delay_timer();           // เริ่มนับ 500ms
        state = ST_WAIT_OFF;                     // เปลี่ยน state
        break;

    case ST_WAIT_OFF:
        /*
         * กำลังรอ 500ms แบบ non-blocking
         *
         * ใน state นี้ main loop สามารถทำงานอื่นได้
         * (UART echo, BLE process, sensor read...)
         *
         * elapse_once(): ใช้ได้กับ Timer oneshot pattern
         * soft_timer_expired(): ใช้ได้กับ SysTick pattern
         *
         * ถ้ายังไม่ครบ → ยังอยู่ใน state นี้
         * ถ้าครบ → ปิด LED, เริ่มรอ 500ms เพื่อเปิดใหม่
         */
        if (elapse_once()) {                    // ครบ 500ms หรือยัง?
            hal_gpio_write(led, 0);              // ปิด LED
            start_nonblock_delay_timer();         // เริ่มนับ 500ms ใหม่
            state = ST_WAIT_ON;                   // เปลี่ยน state
        }
        break;

    case ST_WAIT_ON:
        /*
         * กำลังรอ 500ms ก่อนเปิด LED อีกครั้ง
         * (ให้ LED ดับ 500ms)
         */
        if (elapse_once()) {                    // ครบ 500ms หรือยัง?
            state = ST_LED_OFF;                   // กลับไปเริ่มรอบใหม่
        }
        break;

    default:
        state = ST_LED_OFF;                      // fallback
        break;
    }
}

/*
 * จุดเริ่มต้นโปรแกรม
 */
int main()
{
    hal_uart_handle_t uart;
    uint8_t buf[64];
    uint16_t len;

    /* ตั้งค่านาฬิกา */
    HSECFG_Capacitance(HSECap_18p);
    SetSysClock(CLK_SOURCE_HSE_PLL_100MHz);

    /* เริ่มต้น GPIO สำหรับ LED */
    led = hal_gpio_init(PA11, HAL_GPIO_OUTPUT_PP_5mA);
    hal_gpio_write(led, 0);    // LED ดับตอนเริ่ม

    /* เริ่มต้น UART สำหรับ echo */
    uart = hal_uart_init(115200, UART_DEFAULT_TX_PIN, UART_DEFAULT_RX_PIN);
    hal_uart_send(uart, (const uint8_t *)"Non-blocking demo ready\r\n", 25);

    /*
     * main loop
     *
     * เปรียบเทียบกับ example 00_blink_led:
     *
     * - 00: ใช้ hal_delay_ms(500) → CPU หยุด 500ms
     *       → ไม่สามารถรับ UART ระหว่างรอได้
     *
     * - 14: ใช้ state machine + timer → CPU ไม่หยุด
     *       → รับ UART echo ได้ตลอดเวลา
     *
     *  loop frequency: ~100 kHz (ทำงานเร็วมาก)
     *  แต่ละรอบใช้เวลา ~10µs
     *  ใน 500ms: state machine ทำงาน ~50,000 รอบ
     */
    while (1) {

        /*
         * --- ส่วนที่ 1: State machine สำหรับ LED ---
         *
         * กระพริบ LED แบบ non-blocking
         * ใช้ Timer oneshot (interrupt) เป็นตัวบอกเวลา
         * CPU ไม่หยุด — ทำงานส่วนอื่นได้
         */
        led_state_machine();

        /*
         * --- ส่วนที่ 2: UART echo (non-blocking) ---
         *
         * hal_uart_recv() เป็น non-blocking:
         * - ถ้ามีข้อมูลใน ring buffer → echo กลับ
         * - ถ้าไม่มี → คืน HAL_BUSY ทันที
         *
         * ต่อให้ LED กำลัง "รอ delay" อยู่ UART ก็ทำงานได้
         * เพราะ "การรอ" ในที่นี้คือ state machine polling
         * ไม่ใช่ blocking delay
         */
        if (hal_uart_recv(uart, buf, sizeof(buf), &len) == HAL_OK) {
            hal_uart_send(uart, buf, len);
        }

        /*
         * --- ส่วนที่ 3: เพิ่มเติม ---
         *
         * ในโปรแกรมจริงสามารถเพิ่ม:
         * - hal_ble_process() ถ้าใช้ BLE
         * - อ่าน sensor
         * - ประมวลผลข้อมูล
         * - อื่นๆ
         *
         * ทั้งหมดนี้ทำได้พร้อมกับ LED กระพริบ
         * เพราะไม่มี blocking delay
         */
    }
}
