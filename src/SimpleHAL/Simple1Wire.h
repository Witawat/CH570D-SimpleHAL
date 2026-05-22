#ifndef __SIMPLE_1WIRE_H
#define __SIMPLE_1WIRE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "SimpleHAL.h"
#include <stdint.h>
#include <stdbool.h>

#define ONEWIRE_MAX_BUSES  4

#define ONEWIRE_RESET_PULSE       480
#define ONEWIRE_PRESENCE_WAIT     70
#define ONEWIRE_PRESENCE_TIMEOUT  240
#define ONEWIRE_WRITE_0_LOW       60
#define ONEWIRE_WRITE_1_LOW       10
#define ONEWIRE_WRITE_RECOVERY    1
#define ONEWIRE_READ_LOW          3
#define ONEWIRE_READ_WAIT         10
#define ONEWIRE_READ_RECOVERY     55
#define ONEWIRE_SLOT_TIME         65

#define ONEWIRE_CMD_SKIP_ROM      0xCC
#define ONEWIRE_CMD_READ_ROM      0x33
#define ONEWIRE_CMD_MATCH_ROM     0x55
#define ONEWIRE_CMD_SEARCH_ROM    0xF0
#define ONEWIRE_CMD_ALARM_SEARCH  0xEC

typedef struct {
    uint8_t pin;
    uint8_t rom[8];
    uint8_t last_discrepancy;
    uint8_t last_family_discrepancy;
    bool last_device_flag;
    bool initialized;
} OneWire_Bus;

OneWire_Bus* OneWire_Init(uint8_t pin);
bool OneWire_Reset(OneWire_Bus* bus);
void OneWire_WriteBit(OneWire_Bus* bus, uint8_t bit);
uint8_t OneWire_ReadBit(OneWire_Bus* bus);
void OneWire_WriteByte(OneWire_Bus* bus, uint8_t data);
uint8_t OneWire_ReadByte(OneWire_Bus* bus);
void OneWire_WriteBytes(OneWire_Bus* bus, const uint8_t* data, uint8_t len);
void OneWire_ReadBytes(OneWire_Bus* bus, uint8_t* buffer, uint8_t len);

void OneWire_SkipROM(OneWire_Bus* bus);
bool OneWire_ReadROM(OneWire_Bus* bus, uint8_t* rom);
void OneWire_MatchROM(OneWire_Bus* bus, const uint8_t* rom);
bool OneWire_Select(OneWire_Bus* bus, const uint8_t* rom);

void OneWire_ResetSearch(OneWire_Bus* bus);
bool OneWire_Search(OneWire_Bus* bus);
bool OneWire_AlarmSearch(OneWire_Bus* bus);
void OneWire_GetAddress(OneWire_Bus* bus, uint8_t* rom);

uint8_t OneWire_CRC8(const uint8_t* data, uint8_t len);
bool OneWire_VerifyCRC(const uint8_t* data, uint8_t len);

void OneWire_Depower(OneWire_Bus* bus);
OneWire_Bus* OneWire_GetBusByPin(uint8_t pin);

#ifdef __cplusplus
}
#endif

#endif
