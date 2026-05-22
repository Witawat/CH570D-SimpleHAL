# RF 2.4GHz & BLE

## Overview

โมดูล RF 2.4GHz สำหรับการสื่อสาร proprietary (CH571/CH572) และ BLE 4.x Peripheral (CH572 เท่านั้น) เป็น optional module ที่ต้องเปิดด้วย `HAL_ENABLE_RF=1` หรือ `HAL_ENABLE_BLE=1` ใน `simple_hal_config.h`

## SimpleHAL API

### RF 2.4GHz

| ฟังก์ชัน | คำอธิบาย |
|---|---|
| `hal_rf_init(cfg)` | เริ่มต้น RF |
| `hal_rf_send(data, len)` | ส่งข้อมูล |
| `hal_rf_recv(buf, max_len)` | รับข้อมูล (blocking) |
| `hal_rf_calibrate()` | calibrate RF |

**Config struct (`hal_rf_config_t`):** freq (`uint16_t`, 2400–2483MHz), phy (`PHY_1M`/`PHY_2M`/`PHY_2_4G`), tx_power (`TX_POWER_n25dBm` ถึง `TX_POWER_p6dBm`)

### BLE

| ฟังก์ชัน | คำอธิบาย |
|---|---|
| `hal_ble_init(cfg)` | เริ่มต้น BLE stack |
| `hal_ble_advertise_start()` | เริ่ม advertising |
| `hal_ble_advertise_stop()` | หยุด advertising |
| `hal_ble_send(data, len)` | ส่งข้อมูลไปยัง connected device |
| `hal_ble_is_connected()` | เช็ค connection status |
| `hal_ble_process()` | เรียกใน main loop เพื่อประมวลผล BLE stack |
| `hal_ble_attach_connect_cb(cb)` | callback เมื่อมี device connect |
| `hal_ble_attach_disconnect_cb(cb)` | callback เมื่อ disconnect |
| `hal_ble_attach_data_cb(cb)` | callback เมื่อรับข้อมูล |

**Config struct (`hal_ble_config_t`):** adv_mode (`ADV_CONNECTABLE` / `ADV_NON_CONNECTABLE`), adv_interval_ms, dev_name

## Circuit Guide

### RF 2.4GHz (built-in antenna)

```
CH57x (CH571/572)
  RF_ANT (PA15) ────┬── RF matching network ──── Antenna (PCB trace or ceramic chip antenna)
                     │
                    GND

// ไม่ต้องใช้วงจรเพิ่ม — antenna ภายในบน EVB
// ระยะ: ~30–50m (indoor), ~100m+ (line of sight)
```

### BLE (CH572)

```
CH572
  RF_ANT ──── RF matching ──── Antenna
  GND ─────── ชั้น Ground plane

// ระยะ BLE: ~10–30m (indoor), ~50m+ (open)
```

## CH57x Specifics

### RF 2.4GHz

| PHY | Data Rate | Sensitivity |
|---|---|---|
| 1M | 1 Mbps | -94 dBm |
| 2M | 2 Mbps | -91 dBm |
| 2.4G (proprietary) | 2.4 Mbps | -86 dBm |

**Tx Power:** -25, -23, -20, -18, -15, -12, -9, -6, -3, 0, +3, +6 dBm

### BLE

| BLE Feature | Support |
|---|---|
| Version | BLE 4.2 |
| Role | Peripheral only |
| Max connections | 1 |
| MTU | 23 bytes (default) |
| Advertising channels | 37/38/39 |
| PHY | 1M (BLE 4.x), 2M (optional via lib) |
| Security | pairing, bonding, encryption |
| Throughput | ~2–5 KB/s (practical) |
| Latency | ~6ms (connection interval min) |

## Limitations

| ข้อจำกัด | รายละเอียด |
|---|---|
| BLE = CH572 only | CH570/CH571 ไม่มี BLE (ดูตารางใน README.md) |
| RF = CH571/CH572 only | CH570 ไม่มี RF |
| BLE Peripheral only | ไม่มี Central mode — ไม่สามารถ scan หรือ connect หา device อื่น |
| RF/BLE ไม่สามารถใช้พร้อมกัน | แชร์ฮาร์ดแวร์ RF path เดียวกัน |
| BLE stack เป็น closed-source lib | `libCH572BLE_PERI.a` — ไม่สามารถแก้ไข behavior |
| Antenna matching ต้อง tuned | ถ้าออกแบบ PCB เอง ต้องมี matching network (π-network หรือ L-network) |
| ต้อง process BLE stack บ่อย | `hal_ble_process()` ต้องเรียกใน main loop ทุก ~1–5ms |

## Code สั้น

```c
// RF 2.4GHz — ส่ง "Hello"
hal_rf_config_t rf_cfg = {
    .freq = 2400,
    .phy = PHY_1M,
    .tx_power = TX_POWER_p3dBm
};
hal_rf_init(&rf_cfg);
hal_rf_send((uint8_t*)"Hello", 5);

// BLE — Advertising + Notify
hal_ble_config_t ble_cfg = {
    .adv_mode = ADV_CONNECTABLE,
    .adv_interval_ms = 100,
    .dev_name = "CH572_Device"
};
hal_ble_init(&ble_cfg);
hal_ble_advertise_start();

while (1) {
    hal_ble_process();
    if (hal_ble_is_connected()) {
        hal_ble_send((uint8_t*)"ping", 4);
    }
    Delay_Ms(10);
}
```
