# Memory Map

## System Address Map

| Address | Region | ขนาด | รายละเอียด |
|---|---|---|---|
| `0x00000000`–`0x0003BFFF` | Flash (Code) | 240KB | โปรแกรม + rodata |
| `0x0003C000`–`0x0003FFFF` | Flash (Option/UID) | 16KB | unique ID, option bytes, ISP config |
| `0x20000000`–`0x20002FFF` | RAM | 12KB | data, bss, stack, heap |
| `0x40000000`–`0x4000FFFF` | Peripheral SFR | 64KB | registers ของ peripheral ทั้งหมด |
| `0xE0000000`–`0xE0000FFF` | PFIC | 4KB | Programmable Fast Interrupt Controller |
| `0xE000E010` | SysTick | — | System Tick Timer |

## Flash Layout

```
Address         Content
0x00000000      Interrupt Vector Table (startup)
0x00000004      _start (reset handler)
0x00000100      .highcode (critical code → copy to RAM at boot)
0x00000200      .text (main program)
...
0x0003E000      .data init values (loaded to RAM at boot)
0x0003E800      user data / EEPROM emulation
0x0003F000      reserved
0x0003FFF0      Unique ID (8 bytes)
0x0003FFF8      User Option Byte
0x0003FFFC      reserved
```

**Note:** Flash sector erase = 4KB (0x1000)

## RAM Layout (Linker Script)

```
Address         Content
0x20000000      .data (initialized variables, copied from Flash)
0x20000200      .bss  (zero-initialized)
0x20000500      .heap (dynamic allocation)
0x20000800      .stack (grows downward from end of RAM)
0x20002FFF      Top of RAM
```

**ขนาดตาม Link.ld (CH570):**
- .data: ~512 bytes
- .bss: ~768 bytes
- .heap: ~1KB
- .stack: ~2KB
- **RAM ว่างเหลือ:** ~7–8KB สำหรับ application

## SFR Layout (Partial — เฉพาะที่ SimpleHAL ใช้)

### GPIO (Base: `0x40000000`)

| Address | Register | ขนาด | รายละเอียด |
|---|---|---|---|
| `0x40000000` | `R32_PA_OUT` | 32-bit | Output data |
| `0x40000004` | `R32_PA_CLR` | 32-bit | Clear output |
| `0x40000008` | `R32_PA_SET` | 32-bit | Set output |
| `0x4000000C` | `R32_PA_PIN` | 32-bit | Pin input |
| `0x40000060` | `R16_PA_INT_IF` | 16-bit | Interrupt flag |
| `0x40000064` | `R16_PA_INT_IF_CLR` | 16-bit | Clear interrupt |

### UART (Base: `0x40002000`)

| Address | Register | รายละเอียด |
|---|---|---|
| `0x40002000` | `R8_UART_THR` | Transmit Holding |
| `0x40002000` | `R8_UART_RBR` | Receive Buffer |
| `0x40002004` | `R8_UART_IER` | Interrupt Enable |
| `0x40002008` | `R8_UART_IIR` | Interrupt ID |
| `0x4000200C` | `R8_UART_LSR` | Line Status (bit 5 = THR empty) |
| `0x40002014` | `R8_UART_FCR` | FIFO Control |
| `0x40002024` | `R16_UART_DL` | Divisor Latch (baud = sysclock / (16 × div)) |

### SPI (Base: `0x40001000`)

| Address | Register | รายละเอียด |
|---|---|---|
| `0x40001000` | `R8_SPI_CTRL_MOD` | SPI Control Mode |
| `0x40001004` | `R8_SPI_DATA_BUF` | Data buffer |
| `0x40001008` | `R8_SPI_CLOCK_DIV` | Clock divider |
| `0x4000100C` | `R8_SPI_INTER_EN` | Interrupt enable |
| `0x40001014` | `R8_SPI_INT_FLAG` | Interrupt flag |

### I2C (Base: `0x40003000`)

| Address | Register | รายละเอียด |
|---|---|---|
| `0x40003000` | `R32_I2C_CTRL` | I2C Control |
| `0x40003004` | `R32_I2C_CFG` | I2C Config |
| `0x40003008` | `R32_I2C_SADDR` | Slave address |
| `0x40003020` | `R32_I2C_DATA` | Data register |

### ADC (Base: `0x40007000`)

| Address | Register | รายละเอียด |
|---|---|---|
| `0x40007000` | `R16_ADC_CFG` | ADC Config (bits 0–3 = channel) |
| `0x40007004` | `R16_ADC_DATA` | ADC Data |

### Timer (Base: `0x40004000`)

| Address | Register | รายละเอียด |
|---|---|---|
| `0x40004000` | `R32_TMR_COUNT` | Counter |
| `0x40004004` | `R32_TMR_CNT_END` | Period (end value) |
| `0x40004008` | `R32_TMR_FIFO` | FIFO data |
| `0x40004010` | `R8_TMR_CTRL_MOD` | Control mode |

### PWM (Base: `0x40006000`)

| Address | Register | รายละเอียด |
|---|---|---|
| `0x40006000` | `R8_PWM1_DATA` | PWM1 duty (8-bit) |
| `0x40006004` | `R8_PWM2_DATA` | PWM2 duty (8-bit) |
| `0x40006008` | `R8_PWM3_DATA` | PWM3 duty (8-bit) |
| `0x4000600C` | `R8_PWM4_DATA` | PWM4 duty (8-bit) |
| `0x40006010` | `R8_PWM5_DATA` | PWM5 duty (8-bit) |
| `0x4000601C` | `R16_PWM_CLOCK_DIV` | Clock divider |

### RTC (Base: `0x40008000`)

| Address | Register | รายละเอียด |
|---|---|---|
| `0x40008000` | `R8_RTC_FLAG` | Flag |
| `0x40008004` | `R8_RTC_CTRL_L` | Control Low |
| `0x40008008` | `R8_RTC_CTRL_H` | Control High |
| `NVR` | `RTC_TIME` | 24-byte NVR ที่รักษาค่าเวลา |

### PFIC (Base: `0xE0000000`)

| Address | Register | ขนาด | รายละเอียด |
|---|---|---|---|
| `0xE0000000` | `ISR` | 32-bit | Interrupt Status |
| `0xE0000004` | `IPR` | 32-bit | Interrupt Pending |
| `0xE0000008` | `IENR` | 32-bit | Interrupt Enable |
| `0xE000000C` | `IRER` | 32-bit | Interrupt Reset Enable |
| `0xE0000014` | `IPSR` | 32-bit | Interrupt Software Priority |
| `0xE0000020` | `VTFIDR` | 32-bit | VTF ID |
| `0xE0000030` | `VTFADDR` | 32-bit | VTF Address |

## Interrupt Vector Table

| IRQ # | Handler | Peripheral | SimpleHAL ใช้ |
|---|---|---|---|
| 0 | `handle_reset` | Reset | — |
| 2 | `NMI_Handler` | NMI | — |
| 3 | `HardFault_Handler` | Hard Fault | — |
| 12 | `SysTick_Handler` | SysTick | hal_softimer, Get_CurrentMs/Us |
| 17 | `GPIOA_IRQHandler` | GPIO Interrupt | hal_gpio_attach_irq |
| 19 | `SPI_IRQHandler` | SPI | hal_spi_transfer_dma |
| 20 | `BB_IRQHandler` | BLE Baseband | hal_ble (CH572) |
| 21 | `LLE_IRQHandler` | BLE Link Layer | hal_ble (CH572) |
| 22 | `USB_IRQHandler` | USB | hal_usbdev / hal_usbhost |
| 24 | `TMR_IRQHandler` | Timer | hal_timer |
| 27 | `UART_IRQHandler` | UART | hal_uart RX/TX |
| 28 | `RTC_IRQHandler` | RTC | hal_rtc |
| 29 | `CMP_IRQHandler` | Comparator | hal_cmp |
| 30 | `I2C_IRQHandler` | I2C | hal_i2c |
| 31 | `PWMX_IRQHandler` | PWM | hal_pwm |
| 33 | `KEYSCAN_IRQHandler` | KeyScan | hal_keyscan |
| 34 | `ENCODER_IRQHandler` | Encoder | (ต้องใช้ StdPeriphDriver) |
| 35 | `WDOG_BAT_IRQHandler` | Watchdog / BAT | hal_wdt |
