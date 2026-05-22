/**
 * @file oled_i2c.c
 * @brief OLED I2C Library Implementation
 * @version 1.0
 * @date 2025-12-21
 */

#include "oled_i2c.h"

/* ========== Static Buffers ========== */

// Static buffer for 128x64 (1024 bytes)
static uint8_t oled_buffer_128x64[1024];

// Static buffer for 128x32 (512 bytes)
static uint8_t oled_buffer_128x32[512];

// Static buffer for 64x48 (384 bytes)
static uint8_t oled_buffer_64x48[384];

/* ========== Private Helper Functions ========== */

/**
 * @brief ส่ง Command ไปยัง OLED
 */
void OLED_WriteCommand(OLED_Handle* oled, uint8_t cmd) {
    uint8_t data[2] = {0x00, cmd};  // 0x00 = Command mode
    I2C_Write(oled->i2c_addr, data, 2);
}

/**
 * @brief ส่ง Data ไปยัง OLED
 */
void OLED_WriteData(OLED_Handle* oled, uint8_t data) {
    uint8_t buf[2] = {0x40, data};  // 0x40 = Data mode
    I2C_Write(oled->i2c_addr, buf, 2);
}

/**
 * @brief ส่ง Data หลาย bytes
 */
static void OLED_WriteDataMulti(OLED_Handle* oled, uint8_t* data, uint16_t len) {
    // Send in chunks to avoid I2C buffer overflow
    uint16_t chunk_size = 16;
    uint8_t buf[17];  // 1 byte control + 16 bytes data
    
    for(uint16_t i = 0; i < len; i += chunk_size) {
        uint16_t remaining = len - i;
        uint16_t current_chunk = (remaining < chunk_size) ? remaining : chunk_size;
        
        buf[0] = 0x40;  // Data mode
        memcpy(&buf[1], &data[i], current_chunk);
        I2C_Write(oled->i2c_addr, buf, current_chunk + 1);
    }
}

/**
 * @brief ตั้งค่า Address Window
 */
static void OLED_SetAddressWindow(OLED_Handle* oled, uint8_t x, uint8_t y, uint8_t w, uint8_t h) {
    uint8_t x_end = x + w - 1;
    uint8_t page_start = y / 8;
    uint8_t page_end = (y + h - 1) / 8;
    
    // Column address
    OLED_WriteCommand(oled, SSD1306_COLUMN_ADDR);
    OLED_WriteCommand(oled, x);
    OLED_WriteCommand(oled, x_end);
    
    // Page address
    OLED_WriteCommand(oled, SSD1306_PAGE_ADDR);
    OLED_WriteCommand(oled, page_start);
    OLED_WriteCommand(oled, page_end);
}

/* ========== Initialization ========== */

/**
 * @brief เริ่มต้นการใช้งาน OLED
 */
uint8_t OLED_Init(OLED_Handle* oled, OLED_Size size, uint8_t i2c_addr) {
    // Set parameters based on size
    oled->i2c_addr = i2c_addr;
    oled->buffer_mode = OLED_SINGLE_BUFFER;
    oled->back_buffer = NULL;
    
    switch(size) {
        case OLED_128x64:
            oled->width = 128;
            oled->height = 64;
            oled->pages = 8;
            oled->buffer = oled_buffer_128x64;
            break;
            
        case OLED_128x32:
            oled->width = 128;
            oled->height = 32;
            oled->pages = 4;
            oled->buffer = oled_buffer_128x32;
            break;
            
        case OLED_64x48:
            oled->width = 64;
            oled->height = 48;
            oled->pages = 6;
            oled->buffer = oled_buffer_64x48;
            break;
            
        default:
            return 0;  // Invalid size
    }
    
    // Check if device is ready
    if(!I2C_IsDeviceReady(i2c_addr)) {
        return 0;  // Device not found
    }
    
    // Initialization sequence for SSD1306
    OLED_WriteCommand(oled, SSD1306_DISPLAY_OFF);
    
    // Set display clock divide ratio/oscillator frequency
    OLED_WriteCommand(oled, SSD1306_SET_DISPLAY_CLOCK_DIV);
    OLED_WriteCommand(oled, 0x80);  // Suggested ratio
    
    // Set multiplex ratio
    OLED_WriteCommand(oled, SSD1306_SET_MULTIPLEX_RATIO);
    OLED_WriteCommand(oled, oled->height - 1);
    
    // Set display offset
    OLED_WriteCommand(oled, SSD1306_SET_DISPLAY_OFFSET);
    OLED_WriteCommand(oled, 0x00);  // No offset
    
    // Set start line
    OLED_WriteCommand(oled, SSD1306_SET_START_LINE | 0x00);
    
    // Enable charge pump
    OLED_WriteCommand(oled, SSD1306_CHARGE_PUMP);
    OLED_WriteCommand(oled, 0x14);  // Enable
    
    // Set memory addressing mode
    OLED_WriteCommand(oled, SSD1306_MEMORY_MODE);
    OLED_WriteCommand(oled, 0x00);  // Horizontal addressing mode
    
    // Set segment re-map
    OLED_WriteCommand(oled, SSD1306_SET_SEGMENT_REMAP | 0x01);
    
    // Set COM output scan direction
    OLED_WriteCommand(oled, SSD1306_COM_SCAN_DEC);
    
    // Set COM pins hardware configuration
    OLED_WriteCommand(oled, SSD1306_SET_COM_PINS);
    if(oled->height == 64) {
        OLED_WriteCommand(oled, 0x12);
    } else if(oled->height == 32) {
        OLED_WriteCommand(oled, 0x02);
    } else {
        OLED_WriteCommand(oled, 0x12);
    }
    
    // Set contrast
    OLED_WriteCommand(oled, SSD1306_SET_CONTRAST);
    OLED_WriteCommand(oled, 0x7F);  // Mid contrast
    
    // Set pre-charge period
    OLED_WriteCommand(oled, SSD1306_SET_PRECHARGE);
    OLED_WriteCommand(oled, 0xF1);
    
    // Set VCOMH deselect level
    OLED_WriteCommand(oled, SSD1306_SET_VCOM_DETECT);
    OLED_WriteCommand(oled, 0x40);
    
    // Display all on resume
    OLED_WriteCommand(oled, SSD1306_DISPLAY_ALL_ON_RESUME);
    
    // Normal display (not inverted)
    OLED_WriteCommand(oled, SSD1306_NORMAL_DISPLAY);
    
    // Deactivate scroll
    OLED_WriteCommand(oled, SSD1306_DEACTIVATE_SCROLL);
    
    // Turn on display
    OLED_WriteCommand(oled, SSD1306_DISPLAY_ON);
    
    // Clear buffer
    OLED_Clear(oled);
    OLED_Update(oled);
    
    oled->initialized = 1;
    return 1;
}

/**
 * @brief เปิดใช้งาน Double Buffering
 */
void OLED_EnableDoubleBuffer(OLED_Handle* oled, uint8_t* back_buffer) {
    oled->buffer_mode = OLED_DOUBLE_BUFFER;
    oled->back_buffer = back_buffer;
    
    // Clear back buffer
    uint16_t buffer_size = (oled->width * oled->height) / 8;
    memset(back_buffer, 0, buffer_size);
}

/**
 * @brief สลับ buffer
 */
void OLED_SwapBuffers(OLED_Handle* oled) {
    if(oled->buffer_mode == OLED_DOUBLE_BUFFER && oled->back_buffer != NULL) {
        uint8_t* temp = oled->buffer;
        oled->buffer = oled->back_buffer;
        oled->back_buffer = temp;
    }
}

/* ========== Display Update ========== */

/**
 * @brief อัพเดทหน้าจอทั้งหมด
 */
void OLED_Update(OLED_Handle* oled) {
    OLED_SetAddressWindow(oled, 0, 0, oled->width, oled->height);
    
    uint16_t buffer_size = (oled->width * oled->height) / 8;
    OLED_WriteDataMulti(oled, oled->buffer, buffer_size);
}

/**
 * @brief อัพเดทหน้าจอบางส่วน
 */
void OLED_UpdateArea(OLED_Handle* oled, uint8_t x, uint8_t y, uint8_t w, uint8_t h) {
    // Align to page boundaries
    uint8_t page_start = y / 8;
    uint8_t page_end = (y + h - 1) / 8;
    
    OLED_SetAddressWindow(oled, x, page_start * 8, w, (page_end - page_start + 1) * 8);
    
    // Send data for the specified area
    for(uint8_t page = page_start; page <= page_end; page++) {
        uint16_t offset = page * oled->width + x;
        OLED_WriteDataMulti(oled, &oled->buffer[offset], w);
    }
}

/**
 * @brief ล้างหน้าจอ
 */
void OLED_Clear(OLED_Handle* oled) {
    uint16_t buffer_size = (oled->width * oled->height) / 8;
    memset(oled->buffer, 0, buffer_size);
}

/**
 * @brief เติมหน้าจอด้วยสี
 */
void OLED_Fill(OLED_Handle* oled, OLED_Color color) {
    uint16_t buffer_size = (oled->width * oled->height) / 8;
    uint8_t fill_value = (color == OLED_COLOR_WHITE) ? 0xFF : 0x00;
    memset(oled->buffer, fill_value, buffer_size);
}

/* ========== Display Control ========== */

/**
 * @brief เปิด/ปิดหน้าจอ
 */
void OLED_DisplayOn(OLED_Handle* oled, uint8_t on) {
    OLED_WriteCommand(oled, on ? SSD1306_DISPLAY_ON : SSD1306_DISPLAY_OFF);
}

/**
 * @brief ตั้งค่า Contrast
 */
void OLED_SetContrast(OLED_Handle* oled, uint8_t contrast) {
    OLED_WriteCommand(oled, SSD1306_SET_CONTRAST);
    OLED_WriteCommand(oled, contrast);
}

/**
 * @brief กลับสีหน้าจอ
 */
void OLED_InvertDisplay(OLED_Handle* oled, uint8_t invert) {
    OLED_WriteCommand(oled, invert ? SSD1306_INVERT_DISPLAY : SSD1306_NORMAL_DISPLAY);
}

/* ========== Pixel Operations ========== */

/**
 * @brief ตั้งค่า pixel
 */
void OLED_SetPixel(OLED_Handle* oled, uint8_t x, uint8_t y, OLED_Color color) {
    // Bounds check
    if(x >= oled->width || y >= oled->height) {
        return;
    }
    
    // Calculate buffer position
    uint16_t pos = x + (y / 8) * oled->width;
    uint8_t bit = y % 8;
    
    // Set pixel based on color
    switch(color) {
        case OLED_COLOR_WHITE:
            oled->buffer[pos] |= (1 << bit);
            break;
            
        case OLED_COLOR_BLACK:
            oled->buffer[pos] &= ~(1 << bit);
            break;
            
        case OLED_COLOR_INVERT:
            oled->buffer[pos] ^= (1 << bit);
            break;
    }
}

/**
 * @brief อ่านค่า pixel
 */
uint8_t OLED_GetPixel(OLED_Handle* oled, uint8_t x, uint8_t y) {
    // Bounds check
    if(x >= oled->width || y >= oled->height) {
        return 0;
    }
    
    // Calculate buffer position
    uint16_t pos = x + (y / 8) * oled->width;
    uint8_t bit = y % 8;
    
    return (oled->buffer[pos] & (1 << bit)) ? 1 : 0;
}

/* ========== Hardware Scrolling ========== */

/**
 * @brief เริ่ม Scrolling
 */
void OLED_StartScroll(OLED_Handle* oled, OLED_ScrollDir dir, uint8_t start, uint8_t end, uint8_t interval) {
    OLED_StopScroll(oled);  // Stop any existing scroll
    
    switch(dir) {
        case OLED_SCROLL_RIGHT:
            OLED_WriteCommand(oled, SSD1306_RIGHT_HORIZONTAL_SCROLL);
            OLED_WriteCommand(oled, 0x00);  // Dummy byte
            OLED_WriteCommand(oled, start);
            OLED_WriteCommand(oled, interval);
            OLED_WriteCommand(oled, end);
            OLED_WriteCommand(oled, 0x00);  // Dummy byte
            OLED_WriteCommand(oled, 0xFF);  // Dummy byte
            break;
            
        case OLED_SCROLL_LEFT:
            OLED_WriteCommand(oled, SSD1306_LEFT_HORIZONTAL_SCROLL);
            OLED_WriteCommand(oled, 0x00);  // Dummy byte
            OLED_WriteCommand(oled, start);
            OLED_WriteCommand(oled, interval);
            OLED_WriteCommand(oled, end);
            OLED_WriteCommand(oled, 0x00);  // Dummy byte
            OLED_WriteCommand(oled, 0xFF);  // Dummy byte
            break;
            
        case OLED_SCROLL_DIAG_RIGHT:
            OLED_WriteCommand(oled, SSD1306_SET_VERTICAL_SCROLL_AREA);
            OLED_WriteCommand(oled, 0x00);
            OLED_WriteCommand(oled, oled->height);
            OLED_WriteCommand(oled, SSD1306_VERTICAL_AND_RIGHT_HORIZONTAL_SCROLL);
            OLED_WriteCommand(oled, 0x00);
            OLED_WriteCommand(oled, start);
            OLED_WriteCommand(oled, interval);
            OLED_WriteCommand(oled, end);
            OLED_WriteCommand(oled, 0x01);  // Vertical offset
            break;
            
        case OLED_SCROLL_DIAG_LEFT:
            OLED_WriteCommand(oled, SSD1306_SET_VERTICAL_SCROLL_AREA);
            OLED_WriteCommand(oled, 0x00);
            OLED_WriteCommand(oled, oled->height);
            OLED_WriteCommand(oled, SSD1306_VERTICAL_AND_LEFT_HORIZONTAL_SCROLL);
            OLED_WriteCommand(oled, 0x00);
            OLED_WriteCommand(oled, start);
            OLED_WriteCommand(oled, interval);
            OLED_WriteCommand(oled, end);
            OLED_WriteCommand(oled, 0x01);  // Vertical offset
            break;
    }
    
    OLED_WriteCommand(oled, SSD1306_ACTIVATE_SCROLL);
}

/**
 * @brief หยุด Scrolling
 */
void OLED_StopScroll(OLED_Handle* oled) {
    OLED_WriteCommand(oled, SSD1306_DEACTIVATE_SCROLL);
}
