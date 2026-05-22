/**
 * @file Relay.c
 * @brief Relay Control Library Implementation
 * @version 1.0
 * @date 2026-04-30
 */

#include "Relay.h"

/* ========== Private Helpers ========== */

static void _apply(Relay_Instance* relay, uint8_t on) {
    uint8_t pin_state;
    if (relay->active_level == RELAY_ACTIVE_HIGH) {
        pin_state = on ? HIGH : LOW;
    } else {
        pin_state = on ? LOW : HIGH;
    }
    digitalWrite(relay->pin, pin_state);
    relay->state = on;
}

/* ========== Public Functions ========== */

Relay_Status Relay_Init(Relay_Instance* relay, uint8_t pin, Relay_ActiveLevel level) {
    if (relay == NULL) return RELAY_ERROR_PARAM;

    relay->pin          = pin;
    relay->active_level = level;
    relay->state        = 0;
    relay->initialized  = 0;

    pinMode(pin, PIN_MODE_OUTPUT);

    /* ปิด relay ตอนเริ่มต้น */
    _apply(relay, 0);

    relay->initialized = 1;
    return RELAY_OK;
}

void Relay_On(Relay_Instance* relay) {
    if (relay == NULL || !relay->initialized) return;
    _apply(relay, 1);
}

void Relay_Off(Relay_Instance* relay) {
    if (relay == NULL || !relay->initialized) return;
    _apply(relay, 0);
}

void Relay_Toggle(Relay_Instance* relay) {
    if (relay == NULL || !relay->initialized) return;
    _apply(relay, relay->state ? 0 : 1);
}

void Relay_Set(Relay_Instance* relay, uint8_t state) {
    if (relay == NULL || !relay->initialized) return;
    _apply(relay, state ? 1 : 0);
}

uint8_t Relay_IsOn(Relay_Instance* relay) {
    if (relay == NULL || !relay->initialized) return 0;
    return relay->state;
}
