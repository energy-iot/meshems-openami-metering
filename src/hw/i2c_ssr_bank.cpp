#ifdef ENABLE_RELAYS
#include <Arduino.h>
#include <Wire.h>
#include <hw/i2c_ssr_bank.h>
#include <core/pins.h>

// Many relay/SSR boards are active-low on the expander outputs (LOW = SSR on).
#ifndef I2C_SSR_ACTIVE_LOW
#define I2C_SSR_ACTIVE_LOW 1
#endif

static uint8_t s_shadow = 0x00;
static RelayRule relay_rules[8];

// IoTMug board is reversed: logical channel 0 (Relay 1) maps to bit 7.
static uint8_t logical_to_bit(uint8_t logicalChannel) {
    return (uint8_t)(7u - logicalChannel);
}

static uint8_t to_bus_value(uint8_t logical) {
#if I2C_SSR_ACTIVE_LOW
    return (uint8_t)~logical;
#else
    return logical;
#endif
}

static bool pcf8574_write(uint8_t value) {
    Wire.beginTransmission(PCF8574_I2C_ADDR);
    Wire.write(value);
    return Wire.endTransmission() == 0;
}

static void i2c_scan_bus() {
    Serial.printf("I2C scan SDA=GPIO%d SCL=GPIO%d:\n", I2C_SSR_SDA_GPIO, I2C_SSR_SCL_GPIO);
    int found = 0;
    for (uint8_t addr = 8; addr < 0x78; addr++) {
        Wire.beginTransmission(addr);
        if (Wire.endTransmission() == 0) {
            Serial.printf("  0x%02X ACK\n", addr);
            found++;
        }
    }
    if (!found) {
        Serial.println("  (no devices) - check 4 wires, +5V at SSR, and I2C_SSR_*_GPIO in pins.h");
    }
}

void setup_i2c_ssr_bank() {
    Wire.begin(I2C_SSR_SDA_GPIO, I2C_SSR_SCL_GPIO);
    Wire.setClock(100000);
    i2c_scan_bus();
    s_shadow = 0x00;  // all channels logically off
    if (!pcf8574_write(to_bus_value(s_shadow))) {
        Serial.printf("I2C SSR: no ACK at 0x%02X - set PCF8574_I2C_ADDR to a address from the scan, or fix GPIOs\n",
                      PCF8574_I2C_ADDR);
    } else {
        Serial.println("I2C SSR: PCF8574 ok. Serial: 0-7=toggle, a=all off, ?=help");
    }
}

void loop_i2c_ssr_bank_blink_test() {
    static unsigned long lastBlink = 0;
    if (millis() - lastBlink > 500) {
        lastBlink = millis();
        s_shadow ^= (uint8_t)(1u << 7);
        if (pcf8574_write(to_bus_value(s_shadow))) {
            Serial.printf("SSR1 -> %s\n", (s_shadow & (1u << 7)) ? "ON" : "OFF");
        } else {
            Serial.println("I2C write failed");
        }
    }
}

bool set_i2c_ssr_channel(uint8_t channel, bool on) {
    if (channel > 7) {
        return false;
    }
    uint8_t bit = logical_to_bit(channel);

    if (on) {
        s_shadow |= (uint8_t)(1u << bit);
    } else {
        s_shadow &= (uint8_t)~(1u << bit);
    }

    if (!pcf8574_write(to_bus_value(s_shadow))) {
        Serial.printf("I2C SSR write fail: logical ch=%u bit=%u mask=0x%02X\n", channel, bit, s_shadow);
        return false;
    }
    Serial.printf("I2C SSR set: logical ch=%u bit=%u -> %s (mask=0x%02X)\n", channel, bit, on ? "ON" : "OFF", s_shadow);

    return true;
}

bool get_i2c_ssr_channel(uint8_t channel) {
    if (channel > 7) {
        return false;
    }
    uint8_t bit = logical_to_bit(channel);
    return (s_shadow & (uint8_t)(1u << bit)) != 0;
}

uint8_t get_i2c_ssr_mask() {
    return s_shadow;
}

bool set_relay_rule(uint8_t channel, RelayRule rule) {
    if (channel > 7) return false;
    relay_rules[channel] = rule;
    return true;
}

const RelayRule get_relay_rule(uint8_t channel) {
    if (channel > 7) return RelayRule{};
    return relay_rules[channel];
}

void loop_i2c_ssr_bank_serial() {
    while (Serial.available()) {
        char c = (char)Serial.read();
        if (c >= '0' && c <= '7') {
            int bit = c - '0';
            bool next = !get_i2c_ssr_channel((uint8_t)bit);
            if (set_i2c_ssr_channel((uint8_t)bit, next)) {
                Serial.printf("ch %d -> %s (mask 0x%02X)\n", bit,
                              (s_shadow & (1u << bit)) ? "ON" : "OFF", s_shadow);
            } else {
                Serial.println("I2C write failed");
            }
        } else if (c == 'a' || c == 'A') {
            bool ok = true;
            for (uint8_t channel = 0; channel < 8; channel++) {
                if (!set_i2c_ssr_channel(channel, false)) {
                    ok = false;
                    break;
                }
            }
            if (ok) {
                Serial.println("all channels off");
            }
        } else if (c == '?' || c == 'h') {
            Serial.println("I2C SSR: 0-7 toggle | a all off | ? this help");
        }
    }
}

#endif // ENABLE_RELAYS
