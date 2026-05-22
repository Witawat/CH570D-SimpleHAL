#include "I2CScan.h"
#include "../../SimpleHAL/SimpleHAL.h"

static void _PrintHex2(uint8_t val) {
    I2C_SCAN_PRINTCHAR(((val >> 4) & 0x0F) < 10 ? '0' + ((val >> 4) & 0x0F) : 'a' + ((val >> 4) & 0x0F) - 10);
    I2C_SCAN_PRINTCHAR((val & 0x0F) < 10          ? '0' + (val & 0x0F)        : 'a' + (val & 0x0F) - 10);
}

void I2CScan_Run(void) {
    uint8_t found_count = 0;

    I2C_SCAN_PRINT("I2C_SCAN: Starting I2C scan...\r\n");
    I2C_SCAN_PRINT("     0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f\r\n");

    for (uint8_t row = 0; row < 8; row++) {
        _PrintHex2((uint8_t)(row << 4));
        I2C_SCAN_PRINT(": ");

        for (uint8_t col = 0; col < 16; col++) {
            uint8_t addr = (uint8_t)((row << 4) | col);

            if (addr < 0x08 || addr > 0x77) {
                I2C_SCAN_PRINT("   ");
                continue;
            }

            if (I2C_IsDeviceReady(addr)) {
                I2C_SCAN_PRINT(" ");
                _PrintHex2(addr);
                found_count++;
            } else {
#if I2C_SCAN_SHOW_EMPTY
                I2C_SCAN_PRINT(" --");
#else
                I2C_SCAN_PRINT("   ");
#endif
            }
        }

        I2C_SCAN_PRINT("\r\n");
    }

    I2C_SCAN_PRINT("Scan finished. Found ");
    I2C_SCAN_PRINT_NUM((int32_t)found_count);
    I2C_SCAN_PRINT(" device(s).\r\n");
}
