#pragma once

#include <stdint.h>

/** Per-channel trip thresholds. A value of -1 means the limit is not set and should not be evaluated. */
struct RelayRule {
    float kwh_limit = -1.0f;
    float kw_limit  = -1.0f;
};

/** 8-bit I2C GPIO expander (PCF8574 family) driving an 8-channel SSR / relay bank. */
void setup_i2c_ssr_bank();
/** Poll USB serial: keys 0-7 toggle channels, 'a' all off, '?' help. */
void loop_i2c_ssr_bank_serial();
/** Blink SSR channel 1 on/off every 500ms for testing. */
void loop_i2c_ssr_bank_blink_test();
/** Set channel state (true=on, false=off). Returns false if write fails or channel is invalid. */
bool set_i2c_ssr_channel(uint8_t channel, bool on);
/** Read channel state from local shadow copy (true=on, false=off). */
bool get_i2c_ssr_channel(uint8_t channel);
/** Read all channel states from local shadow mask (bit0=channel0 ... bit7=channel7). */
uint8_t get_i2c_ssr_mask();

/** Store a relay rule for the given channel (0-7). Returns false if channel is out of range. */
bool set_relay_rule(uint8_t channel, RelayRule rule);
/** Retrieve the relay rule for the given channel (0-7). Returns a default (all -1) rule if out of range. */
const RelayRule get_relay_rule(uint8_t channel);
