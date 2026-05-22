/**
 * @file P10.c
 * @brief P10 LED Matrix Display Library Implementation
 * @version 1.0
 * @date 2026-05-03
 */

#include "P10.h"

/* ========== Private Variables ========== */

/**
 * @brief Static instance pointer — timer ISR callback ใช้ access instance
 */
static P10_Instance* _active_instance = NULL;

/**
 * @brief Frame buffer (static allocation)
 *
 * ขนาด buffer = width * rows * P10_NUM_PLANES / 8 bytes
 * สูงสุด: 32*16*3/8 = 192 bytes (RGB)
 */
static uint8_t _framebuffer[32 * 16 * P10_NUM_PLANES / 8];

/* ========== Private Function Prototypes ========== */

/**
 * @brief แปลง SimpleGPIO pin number เป็น P10_PinMap (port + pin mask)
 * @param pin SimpleGPIO pin number (หรือ 0xFF = unused)
 * @return P10_PinMap struct (port=NULL ถ้า pin=0xFF หรือ invalid)
 */
static P10_PinMap P10_ResolvePin(uint8_t pin);

/**
 * @brief Timer ISR callback (wraps P10_ScanHandler)
 */
static void P10_ISRCallback(void);

/* ========== Private Functions ========== */

/**
 * @brief Pin resolution — map pin numbers to ports
 */
static P10_PinMap P10_ResolvePin(uint8_t pin) {
    P10_PinMap result = { NULL, 0 };

    if (pin == 0xFF) return result;  // unused

    // PA1–PA2
    if (pin == 0)  { result.port = GPIOA; result.pin = GPIO_Pin_1; }
    else if (pin == 1)  { result.port = GPIOA; result.pin = GPIO_Pin_2; }
    // PC0–PC7
    else if (pin >= 10 && pin <= 17) {
        result.port = GPIOC;
        result.pin  = (uint16_t)(1 << (pin - 10));
    }
    // PD2–PD7
    else if (pin >= 20 && pin <= 25) {
        result.port = GPIOD;
        result.pin  = (uint16_t)(1 << (pin - 18));
    }

    return result;
}

/**
 * @brief Timer ISR callback
 */
static void P10_ISRCallback(void) {
    if (_active_instance != NULL) {
        P10_ScanHandler(_active_instance);
    }
}

/* ========== Public Functions ========== */

/**
 * @brief เริ่มต้น P10 panel
 */
uint8_t P10_Init(P10_Instance* inst, P10_Config* cfg) {
    // === Null checks ===
    if (inst == NULL) return 0;
    if (cfg  == NULL) return 0;

    // === Validate config ===
    if (cfg->data_r_pin == 0xFF) return 0;  // must have at least R data
    if (cfg->clk_pin    == 0xFF) return 0;
    if (cfg->lat_pin    == 0xFF) return 0;
    if (cfg->oe_pin     == 0xFF) return 0;
    if (cfg->row_a_pin  == 0xFF) return 0;
    if (cfg->row_b_pin  == 0xFF) return 0;
    if (cfg->width  == 0 || cfg->width  > 64) return 0;
    if (cfg->height == 0 || cfg->height > 32) return 0;
    if (cfg->rows   == 0 || cfg->rows   > cfg->height) return 0;
    if (cfg->refresh_rate == 0) return 0;

    // === Copy config ===
    memcpy(&inst->config, cfg, sizeof(P10_Config));

    // === Resolve pin maps ===
    inst->_data_r = P10_ResolvePin(cfg->data_r_pin);
    inst->_data_g = P10_ResolvePin(cfg->data_g_pin);
    inst->_data_b = P10_ResolvePin(cfg->data_b_pin);
    inst->_clk    = P10_ResolvePin(cfg->clk_pin);
    inst->_lat    = P10_ResolvePin(cfg->lat_pin);
    inst->_oe     = P10_ResolvePin(cfg->oe_pin);
    inst->_row_a  = P10_ResolvePin(cfg->row_a_pin);
    inst->_row_b  = P10_ResolvePin(cfg->row_b_pin);
    inst->_row_c  = P10_ResolvePin(cfg->row_c_pin);
    inst->_row_d  = P10_ResolvePin(cfg->row_d_pin);

    // === Calculate buffer size ===
    inst->buffer_size = (cfg->width * cfg->rows * P10_NUM_PLANES) / 8;

    // === Assign frame buffer ===
    inst->framebuffer = _framebuffer;

    // === Clear buffer ===
    memset(inst->framebuffer, 0, inst->buffer_size);

    // === Configure all pins as outputs (initial state LOW) ===
    const uint8_t output_pins[] = {
        cfg->data_r_pin, cfg->clk_pin, cfg->lat_pin, cfg->oe_pin,
        cfg->row_a_pin, cfg->row_b_pin
    };
    pinModeMultiple(output_pins, PIN_MODE_OUTPUT);
    if (cfg->data_g_pin != 0xFF) pinMode(cfg->data_g_pin, PIN_MODE_OUTPUT);
    if (cfg->data_b_pin != 0xFF) pinMode(cfg->data_b_pin, PIN_MODE_OUTPUT);
    if (cfg->row_c_pin  != 0xFF) pinMode(cfg->row_c_pin,  PIN_MODE_OUTPUT);
    if (cfg->row_d_pin  != 0xFF) pinMode(cfg->row_d_pin,  PIN_MODE_OUTPUT);

    // Set initial states
    digitalWrite(cfg->data_r_pin, LOW);
    digitalWrite(cfg->clk_pin,    LOW);
    digitalWrite(cfg->lat_pin,    LOW);
    digitalWrite(cfg->oe_pin,     HIGH);  // OE=HIGH = disable output initially
    digitalWrite(cfg->row_a_pin,  LOW);
    digitalWrite(cfg->row_b_pin,  LOW);
    if (cfg->data_g_pin != 0xFF) digitalWrite(cfg->data_g_pin, LOW);
    if (cfg->data_b_pin != 0xFF) digitalWrite(cfg->data_b_pin, LOW);
    if (cfg->row_c_pin  != 0xFF) digitalWrite(cfg->row_c_pin,  LOW);
    if (cfg->row_d_pin  != 0xFF) digitalWrite(cfg->row_d_pin,  LOW);

    // === Initialize scan state ===
    inst->current_row = 0;

    // === Set active instance for ISR ===
    _active_instance = inst;

    // === Start timer interrupt for scanning ===
    // Timer frequency = refresh_rate * rows
    // e.g., 200 Hz * 16 rows = 3200 Hz
    TIM_SimpleInit(TIM_2, (uint32_t)cfg->refresh_rate * cfg->rows);
    TIM_AttachInterrupt(TIM_2, P10_ISRCallback);
    TIM_Start(TIM_2);

    // === Mark initialized ===
    inst->initialized = 1;

    return 1;
}

/**
 * @brief ตั้งค่าพิกเซล (On/Off)
 */
void P10_SetPixel(P10_Instance* inst, uint8_t x, uint8_t y,
                  uint8_t r, uint8_t g, uint8_t b) {
    uint16_t byte_index;
    uint8_t  bit_mask;

    // === Guard checks ===
    if (inst == NULL) return;
    if (!inst->initialized) return;

    // === Clamp coordinates ===
    if (x >= inst->config.width)  return;
    if (y >= inst->config.height) return;

    // === Calculate bit position ===
#if (P10_COLOR_MODE == P10_MSB_FIRST)
    // MSB-first: bit 7 = first pixel in byte
    byte_index = y * (inst->config.width / 8) + (x / 8);
    bit_mask   = (uint8_t)(1 << (7 - (x & 7)));
#else
    // LSB-first: bit 0 = first pixel in byte
    byte_index = y * (inst->config.width / 8) + (x / 8);
    bit_mask   = (uint8_t)(1 << (x & 7));
#endif

    // === Set/clear bits in appropriate planes ===
    // Plane 0: Red (or single color)
    if (r) {
        inst->framebuffer[byte_index] |= bit_mask;
    } else {
        inst->framebuffer[byte_index] &= (uint8_t)~bit_mask;
    }

#if (P10_COLOR_MODE == P10_DUAL_COLOR || P10_COLOR_MODE == P10_RGB)
    // Plane 1: Green
    uint16_t g_offset = inst->buffer_size / P10_NUM_PLANES;
    if (g) {
        inst->framebuffer[g_offset + byte_index] |= bit_mask;
    } else {
        inst->framebuffer[g_offset + byte_index] &= (uint8_t)~bit_mask;
    }
#endif

#if (P10_COLOR_MODE == P10_RGB)
    // Plane 2: Blue
    uint16_t b_offset = 2 * (inst->buffer_size / P10_NUM_PLANES);
    if (b) {
        inst->framebuffer[b_offset + byte_index] |= bit_mask;
    } else {
        inst->framebuffer[b_offset + byte_index] &= (uint8_t)~bit_mask;
    }
#endif
}

/**
 * @brief ลบ frame buffer (ปิดทุกพิกเซล)
 */
void P10_Clear(P10_Instance* inst) {
    if (inst == NULL) return;
    if (!inst->initialized) return;

    memset(inst->framebuffer, 0, inst->buffer_size);
}

/**
 * @brief เติม frame buffer (เปิดทุกพิกเซลด้วยสี)
 */
void P10_Fill(P10_Instance* inst, uint8_t r, uint8_t g, uint8_t b) {
    uint16_t plane_size;
    uint8_t  fill_val_r;
#if (P10_COLOR_MODE == P10_DUAL_COLOR || P10_COLOR_MODE == P10_RGB)
    uint8_t  fill_val_g;
#endif
#if (P10_COLOR_MODE == P10_RGB)
    uint8_t  fill_val_b;
#endif

    if (inst == NULL) return;
    if (!inst->initialized) return;

    plane_size = inst->buffer_size / P10_NUM_PLANES;

    // Convert 0/1 to 0x00/0xFF
    fill_val_r = r ? 0xFF : 0x00;
#if (P10_COLOR_MODE == P10_DUAL_COLOR || P10_COLOR_MODE == P10_RGB)
    fill_val_g = g ? 0xFF : 0x00;
#endif
#if (P10_COLOR_MODE == P10_RGB)
    fill_val_b = b ? 0xFF : 0x00;
#endif

    // Fill Red plane
    memset(inst->framebuffer, fill_val_r, plane_size);

#if (P10_COLOR_MODE == P10_DUAL_COLOR || P10_COLOR_MODE == P10_RGB)
    // Fill Green plane
    memset(inst->framebuffer + plane_size, fill_val_g, plane_size);
#endif

#if (P10_COLOR_MODE == P10_RGB)
    // Fill Blue plane
    memset(inst->framebuffer + 2 * plane_size, fill_val_b, plane_size);
#endif
}

/**
 * @brief หยุด P10 panel
 */
void P10_Deinit(P10_Instance* inst) {
    if (inst == NULL) return;
    if (!inst->initialized) return;

    // === Stop timer ===
    TIM_Stop(TIM_2);
    TIM_DetachInterrupt(TIM_2);

    // === Clear instance ===
    _active_instance = NULL;
    inst->current_row = 0;
    inst->initialized = 0;
}

/**
 * @brief ISR scan handler — ขับ P10 protocol
 *
 * ฟังก์ชันนี้ถูกเรียกจาก timer ISR ที่ความถี่ refresh_rate × rows Hz
 * ทำหน้าที่ scan หนึ่งแถวต่อครั้ง
 *
 * ⚠ CRITICAL: ใช้ GPIO register writes โดยตรง (no digitalWrite)
 * เพื่อความเร็วสูงสุดใน ISR
 */
void P10_ScanHandler(P10_Instance* inst) {
    uint8_t  row;
    uint16_t row_byte_offset;
    uint8_t  num_data_bytes;
    uint16_t b, bit_idx;
    uint8_t  data_byte_r;
#if (P10_COLOR_MODE == P10_DUAL_COLOR || P10_COLOR_MODE == P10_RGB)
    uint16_t plane_size;
    uint8_t  data_byte_g;
#endif
#if (P10_COLOR_MODE == P10_RGB)
    uint8_t  data_byte_b;
#endif
#if (P10_COLOR_MODE == P10_SINGLE_COLOR)
    uint8_t  bit_val;
#endif

    if (inst == NULL) return;
    if (!inst->initialized) return;

    row = inst->current_row;
#if (P10_COLOR_MODE == P10_DUAL_COLOR || P10_COLOR_MODE == P10_RGB)
    plane_size    = inst->buffer_size / P10_NUM_PLANES;
#endif
    num_data_bytes = inst->config.width / 8;

    // ============================================
    // Step 1: OE = HIGH (disable output)
    // ============================================
    GPIO_SetBits(inst->_oe.port, inst->_oe.pin);

    // ============================================
    // Step 2: Set row address on A/B/C/D pins
    // ============================================
    if (row & 0x01) GPIO_SetBits(inst->_row_a.port, inst->_row_a.pin);
    else            GPIO_ResetBits(inst->_row_a.port, inst->_row_a.pin);

    if (row & 0x02) GPIO_SetBits(inst->_row_b.port, inst->_row_b.pin);
    else            GPIO_ResetBits(inst->_row_b.port, inst->_row_b.pin);

    if (inst->_row_c.port != NULL) {
        if (row & 0x04) GPIO_SetBits(inst->_row_c.port, inst->_row_c.pin);
        else            GPIO_ResetBits(inst->_row_c.port, inst->_row_c.pin);
    }

    if (inst->_row_d.port != NULL) {
        if (row & 0x08) GPIO_SetBits(inst->_row_d.port, inst->_row_d.pin);
        else            GPIO_ResetBits(inst->_row_d.port, inst->_row_d.pin);
    }

    // ============================================
    // Step 3: Shift data for this row
    // ============================================
    row_byte_offset = row * num_data_bytes;

    for (b = 0; b < num_data_bytes; b++) {
        data_byte_r = inst->framebuffer[row_byte_offset + b];

#if (P10_COLOR_MODE == P10_DUAL_COLOR || P10_COLOR_MODE == P10_RGB)
        data_byte_g = inst->framebuffer[plane_size + row_byte_offset + b];
#endif
#if (P10_COLOR_MODE == P10_RGB)
        data_byte_b = inst->framebuffer[2 * plane_size + row_byte_offset + b];
#endif

        // Shift out all 8 bits of this byte
        for (bit_idx = 0; bit_idx < 8; bit_idx++) {
#if (P10_COLOR_MODE == P10_SINGLE_COLOR)
            // --- Single color: just R data pin ---
            if (inst->config.bit_order == P10_MSB_FIRST) {
                bit_val = (data_byte_r & 0x80) ? 1 : 0;
                data_byte_r <<= 1;
            } else {
                bit_val = (data_byte_r & 0x01) ? 1 : 0;
                data_byte_r >>= 1;
            }

            if (bit_val) GPIO_SetBits(inst->_data_r.port, inst->_data_r.pin);
            else         GPIO_ResetBits(inst->_data_r.port, inst->_data_r.pin);

            // CLK toggle
            GPIO_SetBits(inst->_clk.port, inst->_clk.pin);
            GPIO_ResetBits(inst->_clk.port, inst->_clk.pin);

#elif (P10_COLOR_MODE == P10_DUAL_COLOR)
            // --- Dual color: R and G data pins ---
            if (inst->config.bit_order == P10_MSB_FIRST) {
                if (data_byte_r & 0x80) GPIO_SetBits(inst->_data_r.port, inst->_data_r.pin);
                else                    GPIO_ResetBits(inst->_data_r.port, inst->_data_r.pin);
                if (data_byte_g & 0x80) GPIO_SetBits(inst->_data_g.port, inst->_data_g.pin);
                else                    GPIO_ResetBits(inst->_data_g.port, inst->_data_g.pin);
                data_byte_r <<= 1;
                data_byte_g <<= 1;
            } else {
                if (data_byte_r & 0x01) GPIO_SetBits(inst->_data_r.port, inst->_data_r.pin);
                else                    GPIO_ResetBits(inst->_data_r.port, inst->_data_r.pin);
                if (data_byte_g & 0x01) GPIO_SetBits(inst->_data_g.port, inst->_data_g.pin);
                else                    GPIO_ResetBits(inst->_data_g.port, inst->_data_g.pin);
                data_byte_r >>= 1;
                data_byte_g >>= 1;
            }

            // CLK toggle
            GPIO_SetBits(inst->_clk.port, inst->_clk.pin);
            GPIO_ResetBits(inst->_clk.port, inst->_clk.pin);

#elif (P10_COLOR_MODE == P10_RGB)
            // --- RGB: R, G, and B data pins ---
            if (inst->config.bit_order == P10_MSB_FIRST) {
                if (data_byte_r & 0x80) GPIO_SetBits(inst->_data_r.port, inst->_data_r.pin);
                else                    GPIO_ResetBits(inst->_data_r.port, inst->_data_r.pin);
                if (data_byte_g & 0x80) GPIO_SetBits(inst->_data_g.port, inst->_data_g.pin);
                else                    GPIO_ResetBits(inst->_data_g.port, inst->_data_g.pin);
                if (data_byte_b & 0x80) GPIO_SetBits(inst->_data_b.port, inst->_data_b.pin);
                else                    GPIO_ResetBits(inst->_data_b.port, inst->_data_b.pin);
                data_byte_r <<= 1;
                data_byte_g <<= 1;
                data_byte_b <<= 1;
            } else {
                if (data_byte_r & 0x01) GPIO_SetBits(inst->_data_r.port, inst->_data_r.pin);
                else                    GPIO_ResetBits(inst->_data_r.port, inst->_data_r.pin);
                if (data_byte_g & 0x01) GPIO_SetBits(inst->_data_g.port, inst->_data_g.pin);
                else                    GPIO_ResetBits(inst->_data_g.port, inst->_data_g.pin);
                if (data_byte_b & 0x01) GPIO_SetBits(inst->_data_b.port, inst->_data_b.pin);
                else                    GPIO_ResetBits(inst->_data_b.port, inst->_data_b.pin);
                data_byte_r >>= 1;
                data_byte_g >>= 1;
                data_byte_b >>= 1;
            }

            // CLK toggle
            GPIO_SetBits(inst->_clk.port, inst->_clk.pin);
            GPIO_ResetBits(inst->_clk.port, inst->_clk.pin);
#endif
        }
    }

    // ============================================
    // Step 4: Latch (LAT HIGH → LOW)
    // ============================================
    GPIO_SetBits(inst->_lat.port, inst->_lat.pin);
    GPIO_ResetBits(inst->_lat.port, inst->_lat.pin);

    // ============================================
    // Step 5: OE = LOW (enable output)
    // ============================================
    GPIO_ResetBits(inst->_oe.port, inst->_oe.pin);

    // ============================================
    // Step 6: Advance to next row
    // ============================================
    inst->current_row++;
    if (inst->current_row >= inst->config.rows) {
        inst->current_row = 0;
    }
}
